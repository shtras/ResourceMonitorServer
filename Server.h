#pragma once
#include "Metric.h"

#include <thread>

namespace ResourceMonitor
{
class Server
{
public:
    Server(Monitor& monitor);
    void Start();
    void Stop();

    bool Running()
    {
        return true;
    }

private:
    struct ServerImpl;

    Monitor& monitor_;
    std::shared_ptr<ServerImpl> server_ = nullptr;
    std::thread serverThread_;
};
} // namespace ResourceMonitor
