#include "pch.h"

#include "HardwareInfoDll.h"

#include <iostream>

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
    void HardwareInfo::SaveAllHardware() {
        // �M���Ҧ��w��
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            hardware->Update();

            // �M���Ҧ��ǷP��
            for each(ISensor ^ sensor in hardware->Sensors) {
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
        // �M���Ҧ��w��
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            if (hardware->HardwareType == HardwareType::Cpu) {
                hardware->Update();

                cpuInfo->Name = msclr::interop::marshal_as<std::string>(hardware->Name);

                for each(ISensor ^ sensor in hardware->Sensors) {
                    auto sensorValue = sensor->Value;

                    if (!sensorValue.HasValue) continue;
                    std::string sensorName = msclr::interop::marshal_as<std::string>(sensor->Name);

                    // �ھڶǷP�����������ó]�m
                    switch (sensor->SensorType) {
                        case SensorType::Load:
                            if (sensorName == "CPU Total")
                                cpuInfo->CPUUsage = sensorValue.Value;
                            else if (sensorName.find("CPU Core Max") == 0)
                                cpuInfo->MaxCoreUsage = sensorValue.Value;
                            else if (sensorName.find("CPU Core") == 0)
                                cpuInfo->CoreLoad[sensorName] = sensorValue.Value;
                            break;

                        case SensorType::Temperature:
                            if (sensorName == "Core Max")
                                cpuInfo->MaxTemperature = sensorValue.Value;
                            else if (sensorName == "CPU Package")
                                cpuInfo->PackageTemperature = sensorValue.Value;
                            else if (sensorName == "Core Average")
                                cpuInfo->AverageTemperature = sensorValue.Value;
                            else if (sensorName.find("CPU Core") == 0)
                                cpuInfo->CoreTemperature[sensorName] = sensorValue.Value;
                            break;

                        case SensorType::Clock:
                            if (sensorName.find("CPU Core") == 0)
                                cpuInfo->CoreClock[sensorName] = sensorValue.Value;
                            else if (sensorName == "Bus Speed")
                                cpuInfo->BusSpeed = sensorValue.Value;
                            break;

                        case SensorType::Voltage:
                            if (sensorName.find("CPU Core #") == 0)
                                cpuInfo->CoreVoltage[sensorName] = sensorValue.Value;
                            else if (sensorName == "CPU Core")
                                cpuInfo->CPUVoltage = sensorValue.Value;
                            break;

                        case SensorType::Power:
                            if (sensorName == "CPU Package")
                                cpuInfo->PackagePower = sensorValue.Value;
                            else if (sensorName == "CPU Cores")
                                cpuInfo->CoresPower = sensorValue.Value;
                            break;
                    }
                }

                //static const std::regex corePattern("CPU Core #(\\d+)", std::regex_constants::optimize); // ���h��F���Ӥǰt�֤߽s��
                std::bitset<MAX_CORE_NUM> coreExists;

                for (const auto& pair : cpuInfo->CoreLoad) {
                    //std::smatch match;
                    if (pair.first.find("CPU Core #") == 0) { // rfind ��m�� 0 ��ܫe��ǰt
                        int coreNumber = std::stoi(pair.first.substr(10)); // �q�T�w��m�^���Ʀr����
                        if (coreNumber >= 1 && coreNumber <= MAX_CORE_NUM) {
                            coreExists.set(coreNumber - 1);
                        }
                    }
                }

                cpuInfo->Cores = static_cast<int>(coreExists.count());
                cpuInfo->Threads = static_cast<int>(cpuInfo->CoreLoad.size()); // ������ƶq�i�H�����ϥ� CoreLoad ���j�p
            }
        }

        // �ഫ�� JSON �榡
        json result;
        result["Name"] = cpuInfo->Name;
        result["CPUUsage"] = cpuInfo->CPUUsage;
        result["MaxCoreUsage"] = cpuInfo->MaxCoreUsage;
        result["CoreLoad"] = cpuInfo->CoreLoad;
        result["CoreTemperature"] = cpuInfo->CoreTemperature;
        result["CoreVoltage"] = cpuInfo->CoreVoltage;
        result["CoreClock"] = cpuInfo->CoreClock;
        result["MaxTemperature"] = cpuInfo->MaxTemperature;
        result["PackageTemperature"] = cpuInfo->PackageTemperature;
        result["AverageTemperature"] = cpuInfo->AverageTemperature;
        result["BusSpeed"] = cpuInfo->BusSpeed;
        result["CPUVoltage"] = cpuInfo->CPUVoltage;
        result["PackagePower"] = cpuInfo->PackagePower;
        result["CoresPower"] = cpuInfo->CoresPower;
        result["Cores"] = cpuInfo->Cores;
        result["Threads"] = cpuInfo->Threads;

        // �N JSON ����ഫ�� std::string�A�A�ন System::String^
        std::string jsonStr = result.dump(4);
        return msclr::interop::marshal_as<System::String^>(jsonStr);
    }
}