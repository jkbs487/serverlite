#include "slite/TCPServer.h"
#include "slite/EventLoop.h"
#include "slite/Logger.h"
#include "HTTPCodec.h"

#include <cstdio>
#include <map>

using namespace slite;
using namespace std::placeholders;

class HTTPServer
{
public:
    HTTPServer(std::string host, uint16_t port);
    ~HTTPServer();

    void start() {
        server_.start();
        loop_.loop();
    }

    void setThreadNum(int num)
    { server_.setThreadNum(num); }

    HTTPResponse onRequest(HTTPRequest* req);

private:
    EventLoop loop_;
    TCPServer server_;
    HTTPCodec codec_;
};

HTTPServer::HTTPServer(std::string host, uint16_t port)
    : server_(host, port, &loop_, "HTTPServer"),
    codec_(std::bind(&HTTPServer::onRequest, this, std::placeholders::_1))
{
    server_.setMessageCallback(std::bind(&HTTPCodec::onMessage, &codec_, _1, _2, _3));
    //server_.setConnectionCallback();
}

HTTPServer::~HTTPServer()
{
}

HTTPResponse HTTPServer::onRequest(HTTPRequest* req)
{
    HTTPResponse resp;
    std::string body;

    LOG_DEBUG << "request version: " << req->version();
    LOG_DEBUG << "request path: " << req->path();
    LOG_DEBUG << "request method: " << req->method();

    if (req->path() == "/") {
        if (req->method() == HTTPRequest::GET) {
            body += "GET ";
        }
        body += req->version() + "</br>";
        resp.setStatus(HTTPResponse::OK);
        resp.setContentType("text/html");
        body += "<h1>Hello World</h1>";
        resp.setBody(body);
        resp.setContentLength(body.size());
    } else if(req->path() == "/hello") {
        resp.setStatus(HTTPResponse::OK);
        resp.setBody("hello world\n");
        resp.setContentType("text/plain");
        resp.setContentLength(12);
    } else if (req->path() == "/time") {
        time_t rawtime;
        struct tm *info;
        char buffer[80];

        time(&rawtime);    
        info = localtime(&rawtime);
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
        std::string now = std::string(buffer);
        body = now;
        resp.setBody(body);
        resp.setContentLength(body.size());
    } else if (req->path() == "/favicon.ico") {
        resp.setStatus(HTTPResponse::BAD_REQUEST);
    } else {
        body = "<h1>Not Found</h1>";
        resp.setStatus(HTTPResponse::NOT_FOUND);
        resp.setBody(body);
    }
    
    return resp;
}

int main(int argc, char** argv)
{
    Logger::setLogLevel(Logger::DEBUG);

    if (argc != 4) {
        printf("Usage: %s ip port thread\n", argv[0]);
    } else {
        HTTPServer httpServer(argv[1], static_cast<uint16_t>(atoi(argv[2])));
        httpServer.setThreadNum(static_cast<uint16_t>(atoi(argv[3])));
        httpServer.start();
    }
}