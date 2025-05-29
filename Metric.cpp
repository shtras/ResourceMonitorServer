#include "Metric.h"

#include <spdlog/spdlog.h>

#include <Pdh.h>
#include <pdhmsg.h>

namespace ResourceMonitor
{
bool Monitor::Init()
{
    auto status = PdhOpenQuery(nullptr, NULL, &query_);
    if (status != ERROR_SUCCESS) {
        spdlog::error("Failed to open PDH query: {}", status);
        return false;
    }
    counterValueItems_.resize(256);
    return true;
}

bool Monitor::AddCounter(const std::wstring name, const std::string displayName, float multiplier)
{
    auto counter = std::make_shared<Counter>();
    counter->name = name;
    counter->displayName = displayName;
    counter->isArray = name.find(L"*") != std::wstring::npos;
    counter->multiplier = multiplier;
    auto status = PdhAddCounter(query_, name.c_str(), 0, &counter->counter);
    if (status != ERROR_SUCCESS) {
        spdlog::error(L"PdhAddCounter failed for {} with status {}", name, status);
        return false;
    }
    counters_.push_back(counter);
    return true;
}

bool Monitor::AddCustomCounter(std::string displayName)
{
    WCHAR CounterPathBuffer[PDH_MAX_COUNTER_PATH] = L"";
    PDH_BROWSE_DLG_CONFIG BrowseDlgData;
    ZeroMemory(&BrowseDlgData, sizeof(PDH_BROWSE_DLG_CONFIG));
    const auto BROWSE_DIALOG_CAPTION = L"Select a counter to monitor.";

    BrowseDlgData.bIncludeInstanceIndex = FALSE;
    BrowseDlgData.bSingleCounterPerAdd = TRUE;
    BrowseDlgData.bSingleCounterPerDialog = TRUE;
    BrowseDlgData.bLocalCountersOnly = FALSE;
    BrowseDlgData.bWildCardInstances = TRUE;
    BrowseDlgData.bHideDetailBox = TRUE;
    BrowseDlgData.bInitializePath = FALSE;
    BrowseDlgData.bDisableMachineSelection = FALSE;
    BrowseDlgData.bIncludeCostlyObjects = FALSE;
    BrowseDlgData.bShowObjectBrowser = FALSE;
    BrowseDlgData.hWndOwner = NULL;
    BrowseDlgData.szReturnPathBuffer = (LPWSTR)(CounterPathBuffer);
    BrowseDlgData.cchReturnPathLength = PDH_MAX_COUNTER_PATH;
    BrowseDlgData.pCallBack = NULL;
    BrowseDlgData.dwCallBackArg = 0;
    BrowseDlgData.CallBackStatus = ERROR_SUCCESS;
    BrowseDlgData.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
    BrowseDlgData.szDialogBoxCaption = (LPWSTR)BROWSE_DIALOG_CAPTION;

    auto Status = PdhBrowseCounters(&BrowseDlgData);

    if (Status != ERROR_SUCCESS) {
        return false;
    }
    spdlog::info(L"Counter selected: {}", CounterPathBuffer);

    return AddCounter(CounterPathBuffer, displayName);
}

auto Monitor::processArray(const Counter* counter) -> std::expected<float, PDH_STATUS>
{
    DWORD size = counterValueItems_.size() * sizeof PDH_FMT_COUNTERVALUE_ITEM;
    DWORD numItems = 0;
    float totalValue = 0;
    auto status = PdhGetFormattedCounterArray(
        counter->counter, PDH_FMT_DOUBLE, &size, &numItems, counterValueItems_.data()
    );
    if (status == PDH_MORE_DATA) {
        auto requiredItems = size / sizeof(PDH_FMT_COUNTERVALUE_ITEM) + 1;
        spdlog::info("Resizing array to {}", requiredItems);
        counterValueItems_.resize(requiredItems);
        size = counterValueItems_.size() * sizeof PDH_FMT_COUNTERVALUE_ITEM;
        status = PdhGetFormattedCounterArray(
            counter->counter, PDH_FMT_DOUBLE, &size, &numItems, counterValueItems_.data()
        );
        if (status != ERROR_SUCCESS) {
            spdlog::error(L"PdhGetFormattedCounterArray failed with status {}", status);
            return std::unexpected(status);
        }
    }
    for (DWORD i = 0; i < numItems; ++i) {
        totalValue += counterValueItems_[i].FmtValue.doubleValue;
    }
    return totalValue;
}

bool Monitor::process()
{
    auto status = PdhCollectQueryData(query_);
    if (status != ERROR_SUCCESS) {
        spdlog::error(L"PdhCollectQueryData failed with status {}", status);
        return false;
    }
    std::unique_lock l(m_);
    DWORD CounterType;
    PDH_FMT_COUNTERVALUE DisplayValue;
    history_.emplace_back();
    while (history_.size() > 5) {
        history_.pop_front();
    }
    auto& lastMeasurement = history_.back();
    auto unix_timestamp = std::chrono::seconds(std::time(NULL));
    lastMeasurement.timestamp = std::chrono::milliseconds(unix_timestamp).count();
    for (auto& counter : counters_) {
        float value = 0;
        if (counter->isArray) {
            auto maybeValue = processArray(counter.get());
            if (!maybeValue) {
                spdlog::error(
                    L"Failed to process array counter {}: {}", counter->name, maybeValue.error()
                );
                continue;
            }
            value = maybeValue.value();
        } else {
            status = PdhGetFormattedCounterValue(
                counter->counter, PDH_FMT_DOUBLE, &CounterType, &DisplayValue
            );
            if (status != ERROR_SUCCESS) {
                spdlog::error(L"PdhGetFormattedCounterValue failed with status {}", status);
                continue;
            }
            value = DisplayValue.doubleValue;
        }
        lastMeasurement.counters[counter->displayName] = value * counter->multiplier;
    }
    if (lastMeasurement.counters.empty()) {
        history_.pop_back();
    }
    return true;
}

void Monitor::Start()
{
    if (running_) {
        spdlog::error("Trying to start an already running monitor.");
        return;
    }
    running_ = true;
    t_ = std::thread([this] { threadProc(); });
}

void Monitor::Stop()
{
    if (t_.joinable()) {
        running_ = false;
        t_.join();
    }
}

void Monitor::threadProc()
{
    while (running_) {
        Sleep(interval_);

        if (!process()) {
            spdlog::error("Processing counters failed");
            continue;
        }
    }
}

std::list<Measurement> Monitor::GetHistory()
{
    std::unique_lock l(m_);
    std::list<Measurement> res = history_;
    history_.clear();
    return res;
}

Monitor::~Monitor()
{
    if (!query_) {
        return;
    }
    PdhCloseQuery(query_);
}
} // namespace ResourceMonitor
