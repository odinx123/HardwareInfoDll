#pragma once

#include <string>
#include <unordered_map>

#using "LibreHardwareMonitorLib.dll"
using namespace LibreHardwareMonitor::Hardware;

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

    struct NetworkInfo {
        std::string Name;
        double DownloadSpeed = 0.0;  // 下載速度
        double UploadSpeed = 0.0;   // 上傳速度
        double DownloadTotal = 0.0;  // 下載總量
        double UploadTotal = 0.0;  // 上傳總量
        double Utilization = 0.0;  // 網路使用率
    };

	public ref class HardwareInfo {
		Computer^ computer = gcnew Computer();

        CPUInfoDetailed* cpuInfo;
        NetworkInfo* networkInfo;

		// TODO: 請在此新增此類別的方法。
		public:
		HardwareInfo() {
			this->computer->IsCpuEnabled = true;
			this->computer->IsGpuEnabled = true;
			this->computer->IsMemoryEnabled = true;
			this->computer->IsMotherboardEnabled = true;
			this->computer->IsControllerEnabled = true;
			this->computer->IsNetworkEnabled = true;
			this->computer->IsStorageEnabled = true;
			this->computer->Open();

            cpuInfo = new CPUInfoDetailed();
            networkInfo = new NetworkInfo();
		}
		~HardwareInfo() {
			this->computer->Close();
            delete cpuInfo;
            delete networkInfo;
		}

        void SaveAllHardware();  // 保存所有硬體資訊

        System::String^ GetCPUInfo();  // 獲取 CPU 資訊
	};
}
