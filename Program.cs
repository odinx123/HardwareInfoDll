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

            // 計時器
            hardwareInfo.StartSaveAllHardwareThread(50);

            System.Threading.Thread.Sleep(600);  // 暫停 1 秒

            //var temp = hardwareInfo.GetCPUInfo();
            var temp = hardwareInfo.GetMemoryInfo();

            Console.WriteLine(temp);

            //hardwareInfo.PrintAllHardware();

            //// 開始計時
            //long cnt = 0;
            //long testnum = 100;
            //string previousCpuInfo = "";
            //int changeCount = 0;  // 記錄 CPU Info 改變的次數

            //for (int i = 0; i < testnum; i++)
            //{
            //    System.Threading.Thread.Sleep(100);  // 暫停 100 毫秒

            //    Stopwatch stopwatch = Stopwatch.StartNew();

            //    // 獲取 CPU 資訊
            //    string cpuInfo = hardwareInfo.GetCPUInfo();

            //    stopwatch.Stop();

            //    // 比較 cpuInfo 是否改變
            //    if (cpuInfo != previousCpuInfo)
            //    {
            //        changeCount++;  // 記錄變化
            //        previousCpuInfo = cpuInfo;  // 更新上一個 CPU Info
            //    }

            //    cnt += stopwatch.ElapsedMilliseconds * 1000; // 轉換為微秒
            //}

            //// 顯示變化次數
            //Console.WriteLine("CPU Info changed " + changeCount + " times.");

            //// 顯示平均執行時間
            //double avgTime = (double)cnt / testnum;
            //Console.WriteLine("Average Execution Time: " + avgTime.ToString("F2") + " µs");
        }
    }
}
