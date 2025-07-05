#include "HTTPServer.h"
#include "slite/TCPServer.h"
#include "slite/EventLoop.h"
#include "slite/Logger.h"
#include "slite/Logging.h"

#include <cstdio>
#include <map>
#include <filesystem>
#include <fstream>

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
        if (std::filesystem::exists(req->path())) {
            get_file_list(body, req->path());
        } else {
            body += "<h1>404 Not Found</h1>";
            body += "slite";
            resp->setStatus(HTTPResponse::NOT_FOUND);   
            resp->setBody(body);
            resp->setContentLength(body.size());
            return;
        }
    } else {
        body = rules_[req->path()][req->method()]();
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

void HTTPServer::get_file_list(std::string& body, const std::string& path) {
    body += "<!DOCTYPE html>";
    body += "<html lang=\"zh-CN\">";
    try {
        if (std::filesystem::is_directory(path)) {
            for (auto &p : std::filesystem::directory_iterator(path)) {
                LOG_INFO << "body: " << body;
                body += "<a href=" + p.path().string() + ">" + p.path().filename().string() + "</a><br>";
            }
        } else if (std::filesystem::is_regular_file(path)) {
            std::ifstream file(path);
            std::string line;
            if (path.rfind(".png") != std::string::npos) {
                body += "   <img src=" + path + "></img>";
            } else if (path.rfind(".cc") != std::string::npos) {
                body += "<code>";
                while (std::getline(file, line)) {
                    body += line;
                }
                body += "</code>";
            } else {
                while (std::getline(file, line)) {
                    body += "<p>" + line + "</p>";
                }
            }
        }
    } catch(std::exception& e) {
        LOG_ERROR << e.what();
    }
    body += "</html>";
}
