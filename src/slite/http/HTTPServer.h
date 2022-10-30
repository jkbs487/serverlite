#pragma once

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HTTPCodec.h"

#include <functional>
#include <memory>

namespace slite
{

class EventLoop;
class TCPServer;
namespace http
{

using RouteCallback = std::function<std::string()>;
using RouteMap = std::map<std::string, std::map<HTTPMethod, RouteCallback>>;

class HTTPServer
{
public:
    HTTPServer(std::string host, uint16_t port, slite::EventLoop* loop, const std::string& name);
    ~HTTPServer();

    void start();
    void setThreadNum(int num);
    void addRouteCallback(const std::string& rule, HTTPMethod method, const RouteCallback& cb);

private:
    void onRequest(HTTPRequest* req, HTTPResponse* resp);
    void onConnection(const TCPConnectionPtr& conn);

    std::unique_ptr<slite::TCPServer> server_;
    HTTPCodec codec_;
    RouteMap rules_;
};

}
}