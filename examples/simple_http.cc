#include "slite/EventLoop.h"
#include "slite/http/HTTPServer.h"
#include "slite/Logger.h"

#include <functional>

using namespace slite::http;

class SimpleHttpServer
{
public:
    SimpleHttpServer(const std::string& host, uint16_t port, slite::EventLoop* loop)
    : server_(host, port, loop, "SimpleHttpServer") 
    {
        server_.addRouteCallback("/hello", HTTPMethod::GET, std::bind(&SimpleHttpServer::onHello, this));
        server_.addRouteCallback("/", HTTPMethod::GET, std::bind(&SimpleHttpServer::onIndex, this));
        server_.addNotFoundCallback(std::bind(&SimpleHttpServer::onNotFound, this));
    }

    void start()
    {
        server_.start();
    }

private:
    std::string onHello()
    {
        return "<h1>Hello ServerLite</h1>";
    }

    std::string onIndex()
    {
        return "<h1>ServerLite</h1>";
    }

    std::string onNotFound()
    {
        return "<h1>404 Not Found</h1>";
    }

    HTTPServer server_;
};

int main()
{
    slite::Logger::setLogLevel(slite::Logger::DEBUG);
    slite::EventLoop loop;
    SimpleHttpServer httpServer("0.0.0.0", 8080, &loop);
    httpServer.start();
    loop.loop();
}