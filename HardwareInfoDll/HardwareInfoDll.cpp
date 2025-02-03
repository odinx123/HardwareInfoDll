#include "pch.h"

#include "HardwareInfoDll.h"

#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <msclr\marshal_cppstd.h>
#include <bitset>
#include <regex>
#include <locale>
#include <codecvt>
#include <chrono>
#include <thread>
#include <future>

#using "LibreHardwareMonitorLib.dll"
using namespace System;
using namespace System::Threading;
using namespace System::Threading::Tasks;
using namespace System::Collections::Generic;
using namespace LibreHardwareMonitor::Hardware;
using json = nlohmann::json;

#define MAX_CORE_NUM 64
#define DUMP_JSON_INDENT -1  // -1 表示不使用縮排

namespace HardwareInfoDll {
    using HardwareHandler = void(*)(IHardware^, HardwareInfo^);

    // 定義硬體處理函數
    void HandleCPU(IHardware^ hardware, HardwareInfo^ hw) {
        hw->cpuInfo->Name = msclr::interop::marshal_as<std::string>(hardware->Name);

        auto sensors = hardware->Sensors;
        for (int i = 0; i < sensors->Length; ++i) {
            ISensor^ sensor = sensors[i];
            auto sensorValue = sensor->Value;
            if (!sensorValue.HasValue) continue;

            std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);

            // 根據傳感器類型分類並設置
            switch (sensor->SensorType) {
                case SensorType::Load:
                    if (sensorName == "CPU Total")
                        hw->cpuInfo->CPUUsage = sensorValue.Value;
                    else if (sensorName.find("CPU Core Max") == 0)
                        hw->cpuInfo->MaxCoreUsage = sensorValue.Value;
                    else if (sensorName.find("CPU Core") == 0)
                        hw->cpuInfo->CoreLoad[sensorName] = sensorValue.Value;
                    break;

                case SensorType::Temperature:
                    if (sensorName == "Core Max")
                        hw->cpuInfo->MaxTemperature = sensorValue.Value;
                    else if (sensorName == "CPU Package")
                        hw->cpuInfo->PackageTemperature = sensorValue.Value;
                    else if (sensorName == "Core Average")
                        hw->cpuInfo->AverageTemperature = sensorValue.Value;
                    else if (sensorName.find("CPU Core") == 0)
                        hw->cpuInfo->CoreTemperature[sensorName] = sensorValue.Value;
                    break;

                case SensorType::Clock:
                    if (sensorName.find("CPU Core") == 0)
                        hw->cpuInfo->CoreClock[sensorName] = sensorValue.Value;
                    else if (sensorName == "Bus Speed")
                        hw->cpuInfo->BusSpeed = sensorValue.Value;
                    break;

                case SensorType::Voltage:
                    if (sensorName.find("CPU Core #") == 0)
                        hw->cpuInfo->CoreVoltage[sensorName] = sensorValue.Value;
                    else if (sensorName == "CPU Core")
                        hw->cpuInfo->CPUVoltage = sensorValue.Value;
                    break;

                case SensorType::Power:
                    if (sensorName == "CPU Package")
                        hw->cpuInfo->PackagePower = sensorValue.Value;
                    else if (sensorName == "CPU Cores")
                        hw->cpuInfo->CoresPower = sensorValue.Value;
                    break;
            }
        }

        std::bitset<MAX_CORE_NUM> coreExists;

        for (const auto& pair : hw->cpuInfo->CoreLoad) {
            //std::smatch match;
            if (pair.first.find("CPU Core #") == 0) {
                int coreNumber = std::stoi(pair.first.substr(10)); // 從固定位置擷取數字部分
                coreExists.set(coreNumber);
            }
        }

        hw->cpuInfo->Cores = static_cast<int>(coreExists.count());
        hw->cpuInfo->Threads = static_cast<int>(hw->cpuInfo->CoreLoad.size()); // 執行緒數量可以直接使用 CoreLoad 的大小
    }

    void HandleGPU(IHardware^ hardware, HardwareInfo^ hw) {
        auto& gpuMap = *hw->gpuInfoMap;

        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        auto& gpuSensors = gpuMap[hardwareName];

        auto sensors = hardware->Sensors;
        for (int i = 0; i < sensors->Length; ++i) {
            ISensor^ sensor = sensors[i];
            if (!sensor->Value.HasValue) continue;

            std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);
            std::string sensorType = msclr::interop::marshal_as<std::string>(sensor->SensorType.ToString());

            gpuSensors[sensorName] = GpuSensorInfo(sensorType, sensor->Value.Value);
        }
    }

    void HandleMemory(IHardware^ hardware, HardwareInfo^ hw) {
        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        hw->memoryInfo->name = hardwareName;

        auto sensors = hardware->Sensors;
        for (int i = 0; i < sensors->Length; ++i) {
            ISensor^ sensor = sensors[i];
            if (!sensor->Value.HasValue) continue;

            std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);

            if (sensorName == "Memory Used")
                hw->memoryInfo->memoryUsed = sensor->Value.Value;
            else if (sensorName == "Memory Available")
                hw->memoryInfo->memoryAvailable = sensor->Value.Value;
            else if (sensorName == "Memory")
                hw->memoryInfo->memoryUtilization = sensor->Value.Value;
            else if (sensorName == "Virtual Memory Used")
                hw->memoryInfo->virtualMemoryUsed = sensor->Value.Value;
            else if (sensorName == "Virtual Memory Available")
                hw->memoryInfo->virtualMemoryAvailable = sensor->Value.Value;
            else if (sensorName == "Virtual Memory")
                hw->memoryInfo->virtualMemoryUtilization = sensor->Value.Value;
        }
    }

    void HandleStorage(IHardware^ hardware, HardwareInfo^ hw) {
        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        auto& storageMap = *hw->storageInfoMap;
        storageMap[hardwareName] = StorageInfo();

        auto sensors = hardware->Sensors;
        for (int i = 0; i < sensors->Length; ++i) {
            ISensor^ sensor = sensors[i];
            if (!sensor->Value.HasValue) continue;

            std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);

            if (sensorName == "Used Space")
                storageMap[hardwareName].usedSpace = sensor->Value.Value;
            else if (sensorName == "Read Activity")
                storageMap[hardwareName].readActivity = sensor->Value.Value;
            else if (sensorName == "Write Activity")
                storageMap[hardwareName].writeActivity = sensor->Value.Value;
            else if (sensorName == "Total Activity")
                storageMap[hardwareName].totalActivity = sensor->Value.Value;
            else if (sensorName == "Read Rate")
                storageMap[hardwareName].readRate = sensor->Value.Value;
            else if (sensorName == "Write Rate")
                storageMap[hardwareName].writeRate = sensor->Value.Value;
        }
    }

    void HandleNetwork(IHardware^ hardware, HardwareInfo^ hw) {
        std::wstring hardwareName = msclr::interop::marshal_as<std::wstring>(hardware->Name);

        auto& networkMap = *hw->networkInfoMap;
        networkMap[hardwareName] = NetworkInfo();

        auto sensors = hardware->Sensors;
        for (int i = 0; i < sensors->Length; ++i) {
            ISensor^ sensor = sensors[i];
            if (!sensor->Value.HasValue) continue;

            std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);

            if (sensorName == "Data Uploaded")
                networkMap[hardwareName].dataUploaded = sensor->Value.Value;
            else if (sensorName == "Data Downloaded")
                networkMap[hardwareName].dataDownloaded = sensor->Value.Value;
            else if (sensorName == "Upload Speed")
                networkMap[hardwareName].uploadSpeed = sensor->Value.Value;
            else if (sensorName == "Download Speed")
                networkMap[hardwareName].downloadSpeed = sensor->Value.Value;
            else if (sensorName == "Network utilization")
                networkMap[hardwareName].networkUtilization = sensor->Value.Value;
        }
    }

    void HandleBattery(IHardware^ hardware, HardwareInfo^ hw) {
        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        auto sensors = hardware->Sensors;
        for (int i = 0; i < sensors->Length; ++i) {
            ISensor^ sensor = sensors[i];
            if (!sensor->Value.HasValue) continue;

        }
    }

    // 使用 std::unordered_map 來管理硬體處理函數
    std::unordered_map<size_t, HardwareHandler> hardwareHandlers = {
        { (size_t)HardwareType::Cpu, &HandleCPU },
        { (size_t)HardwareType::GpuNvidia, &HandleGPU },
        { (size_t)HardwareType::GpuAmd, &HandleGPU },
        { (size_t)HardwareType::GpuIntel, &HandleGPU },
        { (size_t)HardwareType::Memory, &HandleMemory },
        { (size_t)HardwareType::Storage, &HandleStorage },
        { (size_t)HardwareType::Network, &HandleNetwork }
        //{ (size_t)HardwareType::Battery, &HandleBattery }  // 電池硬體暫時不處理
    };

    // C++/CLI 異步更新實作
    void HardwareInfo::SaveAllHardware() {
        // 主線程處理 CPU/Memory/Storage/Network
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            if (hardware->HardwareType != HardwareType::GpuNvidia &&
                hardware->HardwareType != HardwareType::GpuAmd &&
                hardware->HardwareType != HardwareType::GpuIntel) {
                hardware->Update();
                hardwareHandlers[(size_t)hardware->HardwareType](hardware, this);
            }
        }

        // 使用 Task 處理 GPU 更新（非阻塞）
        Task::Run(gcnew Action(this, &HardwareInfo::UpdateGpuData));
    }

    void HardwareInfo::UpdateGpuData() {
        if (gpuUpdateMutex->WaitOne(0)) {
            for (int i = 0; i < this->computer->Hardware->Count; i++) {
                IHardware^ hardware = this->computer->Hardware[i];
                if (hardware->HardwareType == HardwareType::GpuNvidia ||
                    hardware->HardwareType == HardwareType::GpuAmd ||
                    hardware->HardwareType == HardwareType::GpuIntel) {
                    hardware->Update();
                    hardwareHandlers[(size_t)hardware->HardwareType](hardware, this);
                }
            }
            gpuUpdateMutex->ReleaseMutex();  // 釋放鎖
        }
    }

    void HardwareInfo::PrintAllHardware() {
        // 遍歷所有硬體
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            hardware->Update();

            // 遍歷所有傳感器
            auto sensors = hardware->Sensors;
            for (int i = 0; i < sensors->Length; ++i) {
                ISensor^ sensor = sensors[i];
                auto sensorValue = sensor->Value;
                if (!sensorValue.HasValue) continue;

                std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);
                std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

                // 格式化字串
                System::String^ formattedString = System::String::Format(
                    "Hardware: {0}, HardwareType: {1}, Sensor: {2}, Value: {3}, Type: {4}",
                    gcnew System::String(hardwareName.c_str()),
                    hardware->HardwareType.ToString(),
                    gcnew System::String(sensorName.c_str()),
                    sensorValue.Value.ToString(),
                    sensor->SensorType.ToString());

                // 輸出格式化後的字串
                System::Console::WriteLine(formattedString);
            }
        }
    }

    // 轉換 CPU 資訊結構為 JSON 格式
    System::String^ HardwareInfo::GetCPUInfo() {
        // 轉換為 JSON 格式
        json result = {
            { "Name", cpuInfo->Name },
            { "CPUUsage", cpuInfo->CPUUsage },
            { "MaxCoreUsage", cpuInfo->MaxCoreUsage },
            { "CoreLoad", cpuInfo->CoreLoad },
            { "CoreTemperature", cpuInfo->CoreTemperature },
            { "CoreVoltage", cpuInfo->CoreVoltage },
            { "CoreClock", cpuInfo->CoreClock },
            { "MaxTemperature", cpuInfo->MaxTemperature },
            { "PackageTemperature", cpuInfo->PackageTemperature },
            { "AverageTemperature", cpuInfo->AverageTemperature },
            { "BusSpeed", cpuInfo->BusSpeed },
            { "CPUVoltage", cpuInfo->CPUVoltage },
            { "PackagePower", cpuInfo->PackagePower },
            { "CoresPower", cpuInfo->CoresPower },
            { "Cores", cpuInfo->Cores },
            { "Threads", cpuInfo->Threads }
        };

        // 直接將 JSON 資料轉換成 std::string，再轉成 System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }

    // 轉換 GPU 資訊結構為 JSON 格式
    System::String^ HardwareInfo::GetGPUInfo() {
        // 轉換為 JSON 格式
        json result;

        // 直接在迴圈內處理 GPU 資訊
        for (auto& gpu : *gpuInfoMap) {
            json gpuJson;

            // 將每個感測器的資訊儲存在 JSON 格式中
            for (auto& sensor : gpu.second) {
                gpuJson[sensor.first] = {
                    { "Type", sensor.second.Type },
                    { "Value", sensor.second.Value }
                };
            }

            result[gpu.first] = std::move(gpuJson);  // 使用 std::move 優化
        }

        // 將 JSON 資料轉換成 std::string，再轉成 System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }

    // 轉換記憶體資訊結構為 JSON 格式
    System::String^ HardwareInfo::GetMemoryInfo() {
        // 轉換為 JSON 格式
        json result = {
            { "Name", memoryInfo->name },
            { "MemoryUsed", memoryInfo->memoryUsed },
            { "MemoryAvailable", memoryInfo->memoryAvailable },
            { "MemoryUtilization", memoryInfo->memoryUtilization },
            { "VirtualMemoryUsed", memoryInfo->virtualMemoryUsed },
            { "VirtualMemoryAvailable", memoryInfo->virtualMemoryAvailable },
            { "VirtualMemoryUtilization", memoryInfo->virtualMemoryUtilization }
        };
        // 直接將 JSON 資料轉換成 std::string，再轉成 System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }

    // 轉換儲存資訊結構為 JSON 格式
    System::String^ HardwareInfo::GetStorageInfo() {
        // 轉換為 JSON 格式
        json result;

        // 直接在迴圈內處理儲存資訊
        for (auto& storage : *storageInfoMap) {
            json storageJson = {
                { "UsedSpace", storage.second.usedSpace },
                { "ReadActivity", storage.second.readActivity },
                { "WriteActivity", storage.second.writeActivity },
                { "TotalActivity", storage.second.totalActivity },
                { "ReadRate", storage.second.readRate },
                { "WriteRate", storage.second.writeRate }
            };
            result[storage.first] = std::move(storageJson);
        }

        // 將 JSON 資料轉換成 std::string，再轉成 System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }

    // 轉換網路資訊結構為 JSON 格式
    System::String^ HardwareInfo::GetNetworkInfo() {
        json result;

        try {
            for (auto& network : *networkInfoMap) {
                // 使用 std::wstring_convert 轉換 std::wstring 為 std::string (UTF-8 編碼)
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                std::string networkKey = converter.to_bytes(network.first); // 將 std::wstring 轉換為 std::string

                json networkJson = {
                    { "DataUploaded", network.second.dataUploaded },
                    { "DataDownloaded", network.second.dataDownloaded },
                    { "UploadSpeed", network.second.uploadSpeed },
                    { "DownloadSpeed", network.second.downloadSpeed },
                    { "NetworkUtilization", network.second.networkUtilization }
                };
                result[networkKey] = std::move(networkJson);  // 使用 std::string 作為鍵
                //Console::WriteLine(gcnew String(networkKey.c_str(), 0, networkKey.length(), System::Text::Encoding::UTF8));
            }

            // 序列化為 JSON 字串 (含轉義符號的 UTF-8)
            std::string jsonString = result.dump(DUMP_JSON_INDENT, ' ', true);

            // 返回 C++/CLI 的 System::String^
            return msclr::interop::marshal_as<System::String^>(jsonString);
        }
        catch (const std::exception& e) {
            return "Error during JSON serialization";
        }
    }
}