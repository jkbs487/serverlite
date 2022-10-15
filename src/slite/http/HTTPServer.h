#pragma once

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPCodec.h"

#include <functional>
#include <memory>

using RouteCallback = std::function<std::string()>;

namespace slite
{

class EventLoop;
class TCPServer;
namespace http
{

class HTTPServer
{
public:
    HTTPServer(std::string host, uint16_t port, slite::EventLoop* loop, const std::string& name);
    ~HTTPServer();

    void start();
    void setThreadNum(int num);
    void addRouteCallback(const std::string& rule, HTTPMethod method, const RouteCallback& cb);
    void onRequest(HTTPRequest* req, HTTPResponse* resp);
    void onConnection(const TCPConnectionPtr& conn);

private:
    std::unique_ptr<slite::TCPServer> server_;
    HTTPCodec codec_;

    std::map<std::string, std::map<HTTPMethod, RouteCallback>> rules_;
};

}
}