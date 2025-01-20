#include "pch.h"

#include "HardwareInfoDll.h"

#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <msclr\marshal_cppstd.h>
#include <bitset>

#using "LibreHardwareMonitorLib.dll"
using namespace System;
using namespace System::Threading; // 引入 Thread 類
using namespace LibreHardwareMonitor::Hardware;
using json = nlohmann::json;

#define MAX_CORE_NUM 64
#define DUMP_JSON_INDENT 4

namespace HardwareInfoDll {
    using HardwareHandler = void(*)(IHardware^, HardwareInfo^);
    
    // 定義硬體處理函數
    void HandleCPU(IHardware^ hardware, HardwareInfo^ hw) {
        hw->cpuInfo->Name = msclr::interop::marshal_as<std::string>(hardware->Name);

        for each (ISensor ^ sensor in hardware->Sensors) {
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
    }

    void HandleGPU(IHardware^ hardware, HardwareInfo^ hw) {
        auto& gpuMap = *hw->gpuInfoMap;

        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        // 確保硬體名稱在gpuMap中初始化
        auto& gpuSensors = gpuMap[hardwareName];

        for each (ISensor ^ sensor in hardware->Sensors) {
            if (!sensor->Value.HasValue) continue;

            std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);
            std::string sensorType = msclr::interop::marshal_as<std::string>(sensor->SensorType.ToString());

            gpuSensors[sensorName] = GpuSensorInfo(sensorType, sensor->Value.Value);
        }
    }

    void HandleMemory(IHardware^ hardware, HardwareInfo^ hw) {
        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        for each (ISensor ^ sensor in hardware->Sensors) {
            
        }
    }

    void HandleStorage(IHardware^ hardware, HardwareInfo^ hw) {
        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        for each (ISensor ^ sensor in hardware->Sensors) {

        }
    }

    void HandleNetwork(IHardware^ hardware, HardwareInfo^ hw) {
        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        for each (ISensor ^ sensor in hardware->Sensors) {

        }
    }

    void HandleBattery(IHardware^ hardware, HardwareInfo^ hw) {
        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        for each (ISensor ^ sensor in hardware->Sensors) {

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
        { (size_t)HardwareType::Network, &HandleNetwork },
        { (size_t)HardwareType::Battery, &HandleBattery }
    };

    // 啟動執行緒來保存硬體資訊
    void HardwareInfo::StartSaveAllHardwareThread(int intervalInMilliseconds) {
        // 設定等待時間並啟動執行緒
        this->m_waitInterval = intervalInMilliseconds;

        // 使用 CancellationToken 來允許優雅的停止執行緒
        m_cancellationTokenSource = gcnew System::Threading::CancellationTokenSource();
        Thread^ thread = gcnew Thread(gcnew ParameterizedThreadStart(this, &HardwareInfo::SaveAllHardwareLoop));
        thread->IsBackground = true;  // 設定為背景執行緒，應用程式結束時會自動結束
        thread->Start(m_cancellationTokenSource->Token);  // 傳遞取消標記
    }

    // 保存硬體資訊的執行緒循環
    void HardwareInfo::SaveAllHardwareLoop(Object^ tokenObj) {
        auto token = (System::Threading::CancellationToken)tokenObj;

        try {
            while (!token.IsCancellationRequested) {
                this->SaveAllHardware();  // 保存所有硬體資訊
                Thread::Sleep(this->m_waitInterval);  // 使用設定的等待時間
            }
        }
        catch (Exception^ ex) {
            // 發送取消請求來停止執行緒
            if (!token.IsCancellationRequested) {
                m_cancellationTokenSource->Cancel();  // 停止執行緒
            }
        }
    }

    void HardwareInfo::SaveAllHardware() {
        // 遍歷所有硬體
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            hardware->Update();

            for each (IHardware ^ subHardware in hardware->SubHardware) {
                subHardware->Update();
            }

            if (hardwareHandlers.find((size_t)hardware->HardwareType) != hardwareHandlers.end())  // 如果硬體類型存在於映射中
                hardwareHandlers[(size_t)hardware->HardwareType](hardware, this);  // 呼叫對應的處理函數
            //else {
                //Console::WriteLine("Unknown hardware type: {0}", hardware->HardwareType.ToString());
            //}
        }
    }

    void HardwareInfo::PrintAllHardware() {
        // 遍歷所有硬體
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            hardware->Update();

            for each (IHardware ^ subHardware in hardware->SubHardware) {
                subHardware->Update();
            }

            // 遍歷所有傳感器
            for each (ISensor ^ sensor in hardware->Sensors) {
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

    // 轉換 Network 資訊結構為 JSON 格式
}