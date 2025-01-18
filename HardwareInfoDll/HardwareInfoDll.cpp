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
    // �w�q CPU ��T���c
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

    // �ഫ CPU ��T���c�� JSON �榡
    System::String^ HardwareInfo::GetCPUInfo() {
        CPUInfoDetailed cpuInfo;

        // �M���Ҧ��w��
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            if (hardware->HardwareType == HardwareType::Cpu) {
                hardware->Update();

                cpuInfo.Name = msclr::interop::marshal_as<std::string>(hardware->Name);

                for each(ISensor ^ sensor in hardware->Sensors) {
                    auto sensorValue = sensor->Value;

                    if (!sensorValue.HasValue) continue;
                    std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);

                    // �ھڶǷP�����������ó]�m
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

                //static const std::regex corePattern("CPU Core #(\\d+)", std::regex_constants::optimize); // ���h��F���Ӥǰt�֤߽s��
                std::bitset<MAX_CORE_NUM> coreExists;

                for (const auto& pair : cpuInfo.CoreLoad) {
                    //std::smatch match;
                    if (pair.first.find("CPU Core #") == 0) { // rfind ��m�� 0 ��ܫe��ǰt
                        int coreNumber = std::stoi(pair.first.substr(10)); // �q�T�w��m�^���Ʀr����
                        if (coreNumber >= 1 && coreNumber <= MAX_CORE_NUM) {
                            coreExists.set(coreNumber - 1);
                        }
                    }
                }

                cpuInfo.Cores = static_cast<int>(coreExists.count());
                cpuInfo.Threads = static_cast<int>(cpuInfo.CoreLoad.size()); // ������ƶq�i�H�����ϥ� CoreLoad ���j�p
            }
        }

        // �ഫ�� JSON �榡
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

        // �N JSON ����ഫ�� std::string�A�A�ন System::String^
        std::string jsonStr = result.dump(4);
        return msclr::interop::marshal_as<System::String^>(jsonStr);
        //return gcnew String(jsonStr.c_str());
    }
}