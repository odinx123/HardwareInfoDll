using System;
using System.Diagnostics;  // 引入 Stopwatch
using HardwareInfoDll;  // 引用 C++/CLI DLL

namespace CPUInfoApp
{
    class Program
    {
        static void Main(string[] args)
        {
            // 建立 HardwareInfo 物件
            HardwareInfo hardwareInfo = new HardwareInfo();

            // 開始計時

            // 呼叫 GetCPUInfo 方法並輸出結果
            long cnt = 0;
            for (int i = 0; i < 100; i++)
            {
                Stopwatch stopwatch = Stopwatch.StartNew();
                string cpuInfo = hardwareInfo.GetCPUInfo();
                cnt += stopwatch.ElapsedMilliseconds;
                stopwatch.Stop();
            }
            //Console.WriteLine("CPU Name: " + cpuInfo);

            // 停止計時並顯示執行時間
            Console.WriteLine("Execution Time: " + cnt/100.0 + " ms");
        }
    }
}
