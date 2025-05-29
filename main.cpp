
#include "Metric.h"
#include "Server.h"

#include <spdlog/spdlog.h>

#include <Pdh.h>

#include <array>
#include <memory>

int main()
{
    ResourceMonitor::Monitor m;
    if (!m.Init()) {
        return 1;
    }

    for (int i = 0; i < 16; ++i) {
        m.AddCounter(
            L"\\Processor(" + std::to_wstring(i) + L")\\% Processor Time",
            "CPU" + std::to_string(i)
        );
    }
    m.AddCounter(L"\\Memory\\Available Bytes", "RAM", 1 / 1024.0f / 1024.0f);
    m.AddCounter(L"\\GPU Engine(*_3D)\\Utilization Percentage", "GPU");
    m.AddCounter(L"\\GPU Engine(*_VideoDecode)\\Utilization Percentage", "GPUVD");
    m.AddCounter(L"\\GPU Engine(*_VideoEncode)\\Utilization Percentage", "GPUVE");
    m.AddCounter(L"\\GPU Process Memory(*)\\Total Committed", "GPUMEM", 1 / 1024.0f / 1024.0f);

    //m.AddCustomCounter("Test");
    m.Start();

    ResourceMonitor::Server s(m);
    s.Start();

    while (s.Running()) {
        Sleep(1000);

        //auto hist = m.GetHistory();
        //for (auto& entry : hist) {
        //    for (auto& counter : entry) {
        //        spdlog::info(L"{}: {}", counter.first, counter.second);
        //    }
        //}
    }

    s.Stop();
    m.Stop();

    return 0;
}
