#include "slite/TCPServer.h"
#include "slite/EventLoop.h"
#include "slite/Logger.h"
#include "slite/Logging.h"
#include "HTTPCodec.h"

#include <cstdio>
#include <map>

using namespace slite;
using namespace std::placeholders;

using RouteCallback = std::function<std::string()>;

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

    void addRouteCallback(std::string rule, HTTPMethod method, RouteCallback cb);
    HTTPResponse onRequest(HTTPRequest* req);

private:
    EventLoop loop_;
    TCPServer server_;
    HTTPCodec codec_;

    std::map<std::string, std::map<HTTPRequest, RouteCallback>> rules_;
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

void HTTPServer::addRouteCallback(std::string rule, HTTPMethod method, RouteCallback cb)
{
    rules_[rule][method] = cb;
}

HTTPResponse HTTPServer::onRequest(HTTPRequest* req)
{
    HTTPResponse resp;
    std::string body;

    LOG_DEBUG << "http request version: " << req->version();
    LOG_DEBUG << "http request path: " << req->path();
    LOG_DEBUG << "http request method: " << req->method();

    if (!rules_.count(req->path()) || !rules_[req->path()].count(req->method())) {
        body = "<h1>Not Found</h1>";
        resp.setStatus(HTTPResponse::NOT_FOUND);
        resp.setBody(body);
    }

    resp.setBody(rules_[req->path()][req->method()]);
    resp.setStatus(HTTPResponse::OK);

/*
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
*/

    return resp;
}
/*
int main(int argc, char** argv)
{
    Logger::setLogLevel(Logger::DEBUG);
    Logging* logging = new Logging("http", 1024 * 1024 * 1, true, 3, 1);
    Logger::setOutput([&](const std::string& line) {
        logging->append(line);
    });

    Logger::setFlush([&]() {
        logging->flush();
    });

    if (argc != 4) {
        printf("Usage: %s ip port thread\n", argv[0]);
    } else {
        HTTPServer httpServer(argv[1], static_cast<uint16_t>(atoi(argv[2])));
        httpServer.setThreadNum(static_cast<uint16_t>(atoi(argv[3])));
        httpServer.start();
    }
}
*/