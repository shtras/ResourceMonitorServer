#include "server_http.hpp"

#include "Server.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <codecvt> // codecvt_utf8
#include <locale>  // wstring_convert

namespace ResourceMonitor
{

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
struct Server::ServerImpl
{
    std::unique_ptr<HttpServer> server = std::make_unique<HttpServer>();
};

Server::Server(Monitor& monitor)
    : monitor_(monitor)
{
}

void Server::Start()
{
    spdlog::info("Server thread started.");
    server_ = std::make_shared<ServerImpl>();
    auto& server = server_->server;
    server->config.port = 8700;
    server->resource["^/measurements"]["GET"] = [&](std::shared_ptr<HttpServer::Response> response,
                                            std::shared_ptr<HttpServer::Request> request) {
        spdlog::info("Received request: {}", request->path);
        auto hist = monitor_.GetHistory();
        nlohmann::json j;
        j["measurements"] = nlohmann::json::array();
        for (auto& measurement : hist) {
            auto& jsonEntry = j["measurements"].emplace_back();
            jsonEntry["time"] = measurement.timestamp;
            for (auto& counter : measurement.counters) {
                //jsonEntry[counter.first] = counter.second;
                //spdlog::info(L"{}: {}", counter.first, counter.second);
                    // Replace the line causing the error with the following code:
                jsonEntry.emplace(counter.first, counter.second);
            }
        }
        
        std::stringstream stream;
        stream << j.dump().c_str();
        response->write(stream);
        spdlog::info("Response sent to client. {}", stream.str());
    };
    server->default_resource["GET"] = [&](std::shared_ptr<HttpServer::Response> response,
                                          std::shared_ptr<HttpServer::Request> request) {
        spdlog::info("Received GET request: {}", request->path);
        std::stringstream stream;
        stream << "{\"measurements\":[{\"time\":0,\"CPU0\":50},{\"time\":1,\"CPU0\":75}]}";
        response->write(stream);
        spdlog::info("Response sent to client.");
    };

    server->default_resource["POST"] = [&](std::shared_ptr<HttpServer::Response> response,
                                           std::shared_ptr<HttpServer::Request> request) {
        spdlog::info("Received POST request: {}", request->path);
        std::stringstream stream;
        stream << "Kuku";
        response->write(stream);
        spdlog::info("Response sent to client.");
    };
    serverThread_ = std::thread([&] { server->start(); });
}

void Server::Stop()
{
    auto& server = server_->server;
    server->stop();
    serverThread_.join();
}
} // namespace ResourceMonitor