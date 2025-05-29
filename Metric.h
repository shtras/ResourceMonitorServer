#pragma once

#include <Pdh.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <chrono>
#include <expected>

namespace ResourceMonitor
{
struct Counter
{
    std::wstring name;
    std::string displayName;
    HCOUNTER counter = nullptr;
    bool isArray = false;
    float multiplier = 1.0f;
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
    bool AddCounter(std::wstring name, std::string displayName, float multiplier = 1.0f);
    bool AddCustomCounter(std::string displayName);
    void Start();
    void Stop();
    std::list<Measurement> GetHistory();

private:
    void threadProc();
    bool process();
    auto processArray(const Counter* counter) -> std::expected<float, PDH_STATUS>;


    int interval_ = 2000;
    std::list<std::shared_ptr<Counter>> counters_;
    std::list<Measurement> history_;
    HQUERY query_ = nullptr;
    std::mutex m_;
    std::atomic<bool> running_{false};
    std::thread t_;
    std::vector<PDH_FMT_COUNTERVALUE_ITEM> counterValueItems_;
};
} // namespace ResourceMonitor
