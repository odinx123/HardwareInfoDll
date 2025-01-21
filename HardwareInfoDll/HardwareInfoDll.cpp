#include "pch.h"

#include "HardwareInfoDll.h"

#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <msclr\marshal_cppstd.h>
#include <bitset>

#using "LibreHardwareMonitorLib.dll"
using namespace System;
using namespace System::Threading;
using namespace LibreHardwareMonitor::Hardware;
using json = nlohmann::json;

#define MAX_CORE_NUM 64
#define DUMP_JSON_INDENT 4  // -1 ��ܤ��ϥ��Y��

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

        // �T�O�w��W�٦bgpuMap����l��
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

        auto &storageMap = *hw->storageInfo;
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
        std::string hardwareName = msclr::interop::marshal_as<std::string>(hardware->Name);

        auto sensors = hardware->Sensors;
        for (int i = 0; i < sensors->Length; ++i) {
            ISensor^ sensor = sensors[i];
            if (!sensor->Value.HasValue) continue;

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
        { (size_t)HardwareType::Network, &HandleNetwork },
        { (size_t)HardwareType::Battery, &HandleBattery }
    };

    // �Ұʰ�����ӫO�s�w���T
    void HardwareInfo::StartSaveAllHardwareThread(int intervalInMilliseconds) {
        // �]�w���ݮɶ��ñҰʰ����
        this->m_waitInterval = intervalInMilliseconds;

        // �ϥ� CancellationToken �Ӥ��\�u������������
        m_cancellationTokenSource = gcnew System::Threading::CancellationTokenSource();
        Thread^ thread = gcnew Thread(gcnew ParameterizedThreadStart(this, &HardwareInfo::SaveAllHardwareLoop));
        thread->IsBackground = true;  // �]�w���I��������A���ε{�������ɷ|�۰ʵ���
        thread->Start(m_cancellationTokenSource->Token);  // �ǻ������аO
    }

    // �O�s�w���T��������`��
    void HardwareInfo::SaveAllHardwareLoop(Object^ tokenObj) {
        auto token = (System::Threading::CancellationToken)tokenObj;

        try {
            while (!token.IsCancellationRequested) {
                this->SaveAllHardware();  // �O�s�Ҧ��w���T
                Thread::Sleep(this->m_waitInterval);  // �ϥγ]�w�����ݮɶ�
            }
        }
        catch (Exception^ ex) {
            // �o�e�����ШD�Ӱ�������
            if (!token.IsCancellationRequested) {
                m_cancellationTokenSource->Cancel();  // ��������
            }
        }
    }

    void HardwareInfo::SaveAllHardware() {
        // �M���Ҧ��w��
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            hardware->Update();

            for each (IHardware ^ subHardware in hardware->SubHardware) {
                subHardware->Update();
            }

            if (hardwareHandlers.find((size_t)hardware->HardwareType) != hardwareHandlers.end())  // �p�G�w�������s�b��M�g��
                hardwareHandlers[(size_t)hardware->HardwareType](hardware, this);  // �I�s�������B�z���
            //else {
                //Console::WriteLine("Unknown hardware type: {0}", hardware->HardwareType.ToString());
            //}
        }
    }

    void HardwareInfo::PrintAllHardware() {
        // �M���Ҧ��w��
        for (int i = 0; i < this->computer->Hardware->Count; i++) {
            IHardware^ hardware = this->computer->Hardware[i];
            hardware->Update();

            for each (IHardware ^ subHardware in hardware->SubHardware) {
                subHardware->Update();
            }

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
        for (auto& storage : *storageInfo) {
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

    // �ഫ Network ��T���c�� JSON �榡
    System::String^ HardwareInfo::GetNetworkInfo() {
        // �ഫ�� JSON �榡
        json result;
        // �N JSON ����ഫ�� std::string�A�A�ন System::String^
        return msclr::interop::marshal_as<System::String^>(result.dump(DUMP_JSON_INDENT));
    }
}