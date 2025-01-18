#pragma once

#include <string>

#using "LibreHardwareMonitorLib.dll"
using namespace LibreHardwareMonitor::Hardware;

namespace HardwareInfoDll {
	public ref class HardwareInfo {
		Computer^ computer = gcnew Computer();

		// TODO: 請在此新增此類別的方法。
		public:
		HardwareInfo() {
			this->computer->IsCpuEnabled = true;
			this->computer->IsGpuEnabled = false;
			this->computer->IsMemoryEnabled = false;
			this->computer->IsMotherboardEnabled = false;
			this->computer->IsControllerEnabled = false;
			this->computer->IsNetworkEnabled = false;
			this->computer->IsStorageEnabled = false;
			this->computer->Open();
		}
		~HardwareInfo() {
			this->computer->Close();
		}
		System::String^ GetCPUInfo();
	};
}
