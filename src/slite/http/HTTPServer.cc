#include "HTTPServer.h"
#include "slite/TCPServer.h"
#include "slite/EventLoop.h"
#include "slite/Logger.h"
#include "slite/Logging.h"

#include <cstdio>
#include <map>

using namespace slite::http;
using namespace std::placeholders;

HTTPServer::HTTPServer(std::string host, uint16_t port, slite::EventLoop* loop, const std::string& name)
    : server_(std::make_unique<slite::TCPServer>(host, port, loop, name)),
    codec_(std::bind(&HTTPServer::onRequest, this, _1, _2))
{
    server_->setMessageCallback(std::bind(&HTTPCodec::onMessage, &codec_, _1, _2, _3));
    server_->setConnectionCallback(std::bind(&HTTPServer::onConnection, this, _1));
}

HTTPServer::~HTTPServer()
{
}

void HTTPServer::start() 
{
    server_->start();
}

void HTTPServer::setThreadNum(int num)
{ 
    server_->setThreadNum(num); 
}

void HTTPServer::addRouteCallback(const std::string& rule, HTTPMethod method, const RouteCallback& cb)
{
    rules_[rule][method] = cb;
}

void HTTPServer::addNotFoundCallback(const NotFoundCallback& cb)
{
    notFoundCb_ = cb;
}

void HTTPServer::onRequest(HTTPRequest* req, HTTPResponse* resp)
{
    std::string body;

    if (!rules_.count(req->path()) || !rules_[req->path()].count(req->method())) {
        if (notFoundCb_) {
            body = notFoundCb_();
        } else {
            body = "404 Not Found";
        }
        resp->setStatus(HTTPResponse::NOT_FOUND);
    } else {
        body = rules_[req->path()][req->method()]();
        resp->setStatus(HTTPResponse::OK);
    }
    resp->setBody(body);
    resp->setContentLength(body.size());
}

void HTTPServer::onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        conn->setContext(HTTPRequest());
    }
}