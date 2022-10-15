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
    }

    void start()
    {
        server_.start();
    }

private:
    std::string onHello()
    {
        return "<h1>Hello Slite</h1>";
    }

    HTTPServer server_;
};

int main()
{
    slite::Logger::setLogLevel(slite::Logger::FATAL);
    slite::EventLoop loop;
    SimpleHttpServer httpServer("0.0.0.0", 8080, &loop);
    httpServer.start();
    loop.loop();
}