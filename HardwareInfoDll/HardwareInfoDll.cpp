#include "pch.h"

#include "HardwareInfoDll.h"

#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <msclr\marshal_cppstd.h>
#include <bitset>

#using "LibreHardwareMonitorLib.dll"
using namespace System;
using namespace LibreHardwareMonitor::Hardware;
using json = nlohmann::json;

#define MAX_CORE_NUM 64

namespace HardwareInfoDll {
    // 定義 CPU 資訊結構
    struct CPUInfoDetailed {
        std::string Name;
        float CPUUsage = 0.0;
        float MaxCoreUsage = 0.0;
        std::unordered_map<std::string, float> CoreLoad;
        std::unordered_map<std::string, float> CoreTemperature;
        std::unordered_map<std::string, float> CoreVoltage;
        std::unordered_map<std::string, float> CoreClock;
        float MaxTemperature = 0.0;
        float PackageTemperature = 0.0;
        float AverageTemperature = 0.0;
        float BusSpeed = 0.0;
        float CPUVoltage = 0.0;
        float PackagePower = 0.0;
        float CoresPower = 0.0;
        int Cores = 0;
        int Threads = 0;
    };

    // 轉換 CPU 資訊結構為 JSON 格式
    System::String^ HardwareInfo::GetCPUInfo() {
        CPUInfoDetailed cpuInfo;

        // 遍歷所有硬體
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            if (hardware->HardwareType == HardwareType::Cpu) {
                hardware->Update();

                cpuInfo.Name = msclr::interop::marshal_as<std::string>(hardware->Name);

                for each(ISensor ^ sensor in hardware->Sensors) {
                    auto sensorValue = sensor->Value;

                    if (!sensorValue.HasValue) continue;
                    std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);

                    // 根據傳感器類型分類並設置
                    switch (sensor->SensorType) {
                        case SensorType::Load:
                            if (sensorName == "CPU Total")
                                cpuInfo.CPUUsage = sensorValue.Value;
                            else if (sensorName.find("CPU Core Max") == 0)
                                cpuInfo.MaxCoreUsage = sensorValue.Value;
                            else if (sensorName.find("CPU Core") == 0)
                                cpuInfo.CoreLoad[sensorName] = sensorValue.Value;
                            break;

                        case SensorType::Temperature:
                            if (sensorName == "Core Max")
                                cpuInfo.MaxTemperature = sensorValue.Value;
                            else if (sensorName == "CPU Package")
                                cpuInfo.PackageTemperature = sensorValue.Value;
                            else if (sensorName == "Core Average")
                                cpuInfo.AverageTemperature = sensorValue.Value;
                            else if (sensorName.find("CPU Core") == 0)
                                cpuInfo.CoreTemperature[sensorName] = sensorValue.Value;
                            break;

                        case SensorType::Clock:
                            if (sensorName.find("CPU Core") == 0)
                                cpuInfo.CoreClock[sensorName] = sensorValue.Value;
                            else if (sensorName == "Bus Speed")
                                cpuInfo.BusSpeed = sensorValue.Value;
                            break;

                        case SensorType::Voltage:
                            if (sensorName.find("CPU Core #") == 0)
                                cpuInfo.CoreVoltage[sensorName] = sensorValue.Value;
                            else if (sensorName == "CPU Core")
                                cpuInfo.CPUVoltage = sensorValue.Value;
                            break;

                        case SensorType::Power:
                            if (sensorName == "CPU Package")
                                cpuInfo.PackagePower = sensorValue.Value;
                            else if (sensorName == "CPU Cores")
                                cpuInfo.CoresPower = sensorValue.Value;
                            break;
                    }
                }

                //static const std::regex corePattern("CPU Core #(\\d+)", std::regex_constants::optimize); // 正則表達式來匹配核心編號
                std::bitset<MAX_CORE_NUM> coreExists;

                for (const auto& pair : cpuInfo.CoreLoad) {
                    //std::smatch match;
                    if (pair.first.find("CPU Core #") == 0) { // rfind 位置為 0 表示前綴匹配
                        int coreNumber = std::stoi(pair.first.substr(10)); // 從固定位置擷取數字部分
                        if (coreNumber >= 1 && coreNumber <= MAX_CORE_NUM) {
                            coreExists.set(coreNumber - 1);
                        }
                    }
                }

                cpuInfo.Cores = static_cast<int>(coreExists.count());
                cpuInfo.Threads = static_cast<int>(cpuInfo.CoreLoad.size()); // 執行緒數量可以直接使用 CoreLoad 的大小
            }
        }

        // 轉換為 JSON 格式
        json result;
        result["Name"] = cpuInfo.Name;
        result["CPUUsage"] = cpuInfo.CPUUsage;
        result["MaxCoreUsage"] = cpuInfo.MaxCoreUsage;
        result["CoreLoad"] = cpuInfo.CoreLoad;
        result["CoreTemperature"] = cpuInfo.CoreTemperature;
        result["CoreVoltage"] = cpuInfo.CoreVoltage;
        result["CoreClock"] = cpuInfo.CoreClock;
        result["MaxTemperature"] = cpuInfo.MaxTemperature;
        result["PackageTemperature"] = cpuInfo.PackageTemperature;
        result["AverageTemperature"] = cpuInfo.AverageTemperature;
        result["BusSpeed"] = cpuInfo.BusSpeed;
        result["CPUVoltage"] = cpuInfo.CPUVoltage;
        result["PackagePower"] = cpuInfo.PackagePower;
        result["CoresPower"] = cpuInfo.CoresPower;
        result["Cores"] = cpuInfo.Cores;
        result["Threads"] = cpuInfo.Threads;

        // 將 JSON 資料轉換成 std::string，再轉成 System::String^
        std::string jsonStr = result.dump(4);
        return msclr::interop::marshal_as<System::String^>(jsonStr);
        //return gcnew String(jsonStr.c_str());
    }
}