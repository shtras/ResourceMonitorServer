#pragma once

#include <Pdh.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <chrono>

namespace ResourceMonitor
{
struct Counter
{
    std::wstring name;
    std::string displayName;
    HCOUNTER counter = nullptr;
};

struct Measurement
{
    std::map<std::string, int> counters;
    uint64_t timestamp;
};

class Monitor
{
public:
    ~Monitor();
    bool Init();
    bool AddCounter(std::wstring name, std::string displayName);
    bool AddCustomCounter(std::string displayName);
    void Start();
    void Stop();
    std::list<Measurement> GetHistory();

private:
    void threadProc();
    bool process();


    int interval_ = 2000;
    std::list<std::shared_ptr<Counter>> counters_;
    std::list<Measurement> history_;
    HQUERY query_ = nullptr;
    std::mutex m_;
    std::atomic<bool> running_{false};
    std::thread t_;
};
} // namespace ResourceMonitor
