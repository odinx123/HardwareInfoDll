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
#define DUMP_JSON_INDENT -1  // -1 ��ܤ��ϥ��Y��

namespace HardwareInfoDll {
    using HardwareHandler = void(*)(IHardware^, HardwareInfo^);

    // �w�q�w��B�z���
    void HandleCPU(IHardware^ hardware, HardwareInfo^ hw) {
        hw->cpuInfo->Name = msclr::interop::marshal_as<std::string>(hardware->Name);

        auto sensors = hardware->Sensors;
        for (int i = 0; i < sensors->Length; ++i) {
            ISensor^ sensor = sensors[i];
            auto sensorValue = sensor->Value;
            if (!sensorValue.HasValue) continue;

            std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);

            // �ھڶǷP�����������ó]�m
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
                int coreNumber = std::stoi(pair.first.substr(10)); // �q�T�w��m�^���Ʀr����
                coreExists.set(coreNumber);
            }
        }

        hw->cpuInfo->Cores = static_cast<int>(coreExists.count());
        hw->cpuInfo->Threads = static_cast<int>(hw->cpuInfo->CoreLoad.size()); // ������ƶq�i�H�����ϥ� CoreLoad ���j�p
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

    // �ϥ� std::unordered_map �Ӻ޲z�w��B�z���
    std::unordered_map<size_t, HardwareHandler> hardwareHandlers = {
        { (size_t)HardwareType::Cpu, &HandleCPU },
        { (size_t)HardwareType::GpuNvidia, &HandleGPU },
        { (size_t)HardwareType::GpuAmd, &HandleGPU },
        { (size_t)HardwareType::GpuIntel, &HandleGPU },
        { (size_t)HardwareType::Memory, &HandleMemory },
        { (size_t)HardwareType::Storage, &HandleStorage },
        { (size_t)HardwareType::Network, &HandleNetwork }
        //{ (size_t)HardwareType::Battery, &HandleBattery }  // �q���w��Ȯɤ��B�z
    };

    // C++/CLI ���B��s��@
    void HardwareInfo::SaveAllHardware() {
        // �D�u�{�B�z CPU/Memory/Storage/Network
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            if (hardware->HardwareType != HardwareType::GpuNvidia &&
                hardware->HardwareType != HardwareType::GpuAmd &&
                hardware->HardwareType != HardwareType::GpuIntel) {
                hardware->Update();
                hardwareHandlers[(size_t)hardware->HardwareType](hardware, this);
            }
        }

        // �ϥ� Task �B�z GPU ��s�]�D����^
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
            gpuUpdateMutex->ReleaseMutex();  // ������
        }
    }

    void HardwareInfo::PrintAllHardware() {
        // �M���Ҧ��w��
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            hardware->Update();

            // �M���Ҧ��ǷP��
            auto sensors = hardware->Sensors;
            for (int i = 0; i < sensors->Length; ++i) {
                ISensor^ sensor = sensors[i];
                auto sensorValue = sensor->Value;
                if (!sensorValue.HasValue) continue;

                std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);
                std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

                // �榡�Ʀr��
                System::String^ formattedString = System::String::Format(
                    "Hardware: {0}, HardwareType: {1}, Sensor: {2}, Value: {3}, Type: {4}",
                    gcnew System::String(hardwareName.c_str()),
                    hardware->HardwareType.ToString(),
                    gcnew System::String(sensorName.c_str()),
                    sensorValue.Value.ToString(),
                    sensor->SensorType.ToString());

                // ��X�榡�ƫ᪺�r��
                System::Console::WriteLine(formattedString);
            }
        }
    }

    // �ഫ CPU ��T���c�� JSON �榡
    System::String^ HardwareInfo::GetCPUInfo() {
        // �ഫ�� JSON �榡
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

        // �����N JSON ����ഫ�� std::string�A�A�ন System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }

    // �ഫ GPU ��T���c�� JSON �榡
    System::String^ HardwareInfo::GetGPUInfo() {
        // �ഫ�� JSON �榡
        json result;

        // �����b�j�餺�B�z GPU ��T
        for (auto& gpu : *gpuInfoMap) {
            json gpuJson;

            // �N�C�ӷP��������T�x�s�b JSON �榡��
            for (auto& sensor : gpu.second) {
                gpuJson[sensor.first] = {
                    { "Type", sensor.second.Type },
                    { "Value", sensor.second.Value }
                };
            }

            result[gpu.first] = std::move(gpuJson);  // �ϥ� std::move �u��
        }

        // �N JSON ����ഫ�� std::string�A�A�ন System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }

    // �ഫ�O�����T���c�� JSON �榡
    System::String^ HardwareInfo::GetMemoryInfo() {
        // �ഫ�� JSON �榡
        json result = {
            { "Name", memoryInfo->name },
            { "MemoryUsed", memoryInfo->memoryUsed },
            { "MemoryAvailable", memoryInfo->memoryAvailable },
            { "MemoryUtilization", memoryInfo->memoryUtilization },
            { "VirtualMemoryUsed", memoryInfo->virtualMemoryUsed },
            { "VirtualMemoryAvailable", memoryInfo->virtualMemoryAvailable },
            { "VirtualMemoryUtilization", memoryInfo->virtualMemoryUtilization }
        };
        // �����N JSON ����ഫ�� std::string�A�A�ন System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }

    // �ഫ�x�s��T���c�� JSON �榡
    System::String^ HardwareInfo::GetStorageInfo() {
        // �ഫ�� JSON �榡
        json result;

        // �����b�j�餺�B�z�x�s��T
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

        // �N JSON ����ഫ�� std::string�A�A�ন System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }

    // �ഫ������T���c�� JSON �榡
    System::String^ HardwareInfo::GetNetworkInfo() {
        json result;

        try {
            for (auto& network : *networkInfoMap) {
                // �ϥ� std::wstring_convert �ഫ std::wstring �� std::string (UTF-8 �s�X)
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                std::string networkKey = converter.to_bytes(network.first); // �N std::wstring �ഫ�� std::string

                json networkJson = {
                    { "DataUploaded", network.second.dataUploaded },
                    { "DataDownloaded", network.second.dataDownloaded },
                    { "UploadSpeed", network.second.uploadSpeed },
                    { "DownloadSpeed", network.second.downloadSpeed },
                    { "NetworkUtilization", network.second.networkUtilization }
                };
                result[networkKey] = std::move(networkJson);  // �ϥ� std::string �@����
                //Console::WriteLine(gcnew String(networkKey.c_str(), 0, networkKey.length(), System::Text::Encoding::UTF8));
            }

            // �ǦC�Ƭ� JSON �r�� (�t��q�Ÿ��� UTF-8)
            std::string jsonString = result.dump(DUMP_JSON_INDENT, ' ', true);

            // ��^ C++/CLI �� System::String^
            return msclr::interop::marshal_as<System::String^>(jsonString);
        }
        catch (const std::exception& e) {
            return "Error during JSON serialization";
        }
    }
}