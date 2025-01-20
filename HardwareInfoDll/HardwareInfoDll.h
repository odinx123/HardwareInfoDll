#pragma once

#include <string>
#include <unordered_map>

#using "LibreHardwareMonitorLib.dll"
using namespace LibreHardwareMonitor::Hardware;

namespace HardwareInfoDll {
    public ref class UpdateVisitor : public LibreHardwareMonitor::Hardware::IVisitor {
        public:
        virtual void VisitComputer(LibreHardwareMonitor::Hardware::IComputer^ computer) {
            computer->Traverse(this); // 遍歷所有硬體並調用訪問者模式
        }

        virtual void VisitHardware(LibreHardwareMonitor::Hardware::IHardware^ hardware) {
            hardware->Update(); // 更新硬體資訊

            // 遍歷所有子硬體並接受訪問者
            for each (LibreHardwareMonitor::Hardware::IHardware ^ subHardware in hardware->SubHardware) {
                subHardware->Accept(this); // 呼叫子硬體的 Accept 方法
            }
        }

        virtual void VisitSensor(LibreHardwareMonitor::Hardware::ISensor^ sensor) {}
        virtual void VisitParameter(LibreHardwareMonitor::Hardware::IParameter^ parameter) {}
    };


    // 定義 CPU 資訊結構
    struct CpuInfo {
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

    struct GpuSensorInfo {
        std::string Type;
        float Value = 0.0;

        GpuSensorInfo() {}
        GpuSensorInfo(const std::string& type, float value) : Type(type), Value(value) {}
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
        int m_waitInterval;  // 用來儲存等待時間（毫秒）
        System::Threading::CancellationTokenSource^ m_cancellationTokenSource;  // 用來取消執行緒的 CancellationTokenSource

        // 定義硬體處理函數
        public:
        CpuInfo* cpuInfo;
        std::unordered_map<std::string, std::unordered_map<std::string, GpuSensorInfo>>* gpuInfoMap;
        std::unordered_map <std::string, NetworkInfo>* networkInfoMap;

		// TODO: 請在此新增此類別的方法。
		public:
		HardwareInfo() {
			this->computer->IsCpuEnabled = true;
			this->computer->IsGpuEnabled = true;
			this->computer->IsMemoryEnabled = true;
			this->computer->IsMotherboardEnabled = true;
			this->computer->IsNetworkEnabled = true;
			this->computer->IsStorageEnabled = true;
            this->computer->IsPsuEnabled = true;
            this->computer->IsBatteryEnabled = true;
			this->computer->Open();
            this->computer->Accept(gcnew UpdateVisitor());

            cpuInfo = new CpuInfo();
            gpuInfoMap = new std::unordered_map<std::string, std::unordered_map<std::string, GpuSensorInfo>>();
            networkInfoMap = new std::unordered_map<std::string, NetworkInfo>();
		}
        ~HardwareInfo() {
            this->computer->Close();

            delete cpuInfo;
            delete gpuInfoMap;
            delete networkInfoMap;
        }

        void StartSaveAllHardwareThread(int intervalInMilliseconds);  // 開始保存所有硬體資訊的執行緒

        void SaveAllHardwareLoop(Object^ tokenObj);  // 保存所有硬體資訊的循環

        void PrintAllHardware();  // 保存所有硬體資訊

        void SaveAllHardware();  // 保存所有硬體資訊

        System::String^ GetCPUInfo();  // 獲取 CPU 資訊

        System::String^ GetGPUInfo();  // 獲取 GPU 資訊
	};
}
