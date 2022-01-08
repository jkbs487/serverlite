#include "TCPServer.h"
#include "EventLoop.h"
#include "Logger.h"

#include <cstdio>
#include <map>

using namespace tcpserver;

class HTTPRequest
{
public:
    enum HTTPMethod {
        GET,
        POST
    };
    
    HTTPRequest(std::string content);
    ~HTTPRequest() {};

    HTTPMethod method() 
    { return method_; }

    std::string path()
    { return path_; }

    std::string version()
    { return version_; }

    std::string getHeader(std::string field);

private:
    void parseHttpRequest(std::string content);

    HTTPMethod method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> header_;
};

HTTPRequest::HTTPRequest(std::string content)
{
    parseHttpRequest(content);
}

std::string HTTPRequest::getHeader(std::string field)
{
    return header_[field];
}

void HTTPRequest::parseHttpRequest(std::string buffer)
{
    std::string requestLine;
    requestLine = buffer.substr(0, buffer.find_first_of("\r\n"));
    size_t curPos = requestLine.find_first_of(" ");
    method_ = requestLine.substr(0, curPos) == "GET" ? GET : POST;

    size_t prevPos = curPos;
    curPos = requestLine.find_first_of(" ", prevPos+1);
    path_ = requestLine.substr(prevPos + 1, curPos - prevPos - 1);

    prevPos = requestLine.find_last_of(" ");
    curPos = buffer.find_first_of("\r\n");
    version_ = requestLine.substr(prevPos + 1, curPos - prevPos - 1);

    size_t i = curPos;
    while (i <= buffer.size()-2) {
        curPos = buffer.find_first_of("\r\n", i+2);
        std::string headerLine = buffer.substr(i+2, curPos - i - 1);
        i = curPos;

        std::string field = headerLine.substr(0, headerLine.find_first_of(":"));
        if (!field.empty())
            header_[field] = headerLine.substr(headerLine.find_first_of(":")+2, headerLine.size()-field.size()-2);
    }
}

class HTTPResponse
{
public:
    enum HTTPStatus {
        OK = 200,
        BAD_REQUEST = 400,
        SERVER_ERROR = 500
    };

    HTTPResponse() {};
    ~HTTPResponse() {};

    HTTPStatus status()
    { return status_; }
    void setVersion(std::string version)
    { version_ = version; }
    void setStatus(HTTPStatus status)
    { status_ = status; }
    void setContentLength(size_t length)
    { length_ = length; }
    void setContentType(std::string type)
    { type_ = type; }
    void setHeader(std::string field, std::string value)
    { header_[field] = value; }

    void setBody(const std::string& body)
    { body_ = body; }

    std::string toString();
private:
    std::string statusToString(HTTPStatus status);

    std::string version_;
    HTTPStatus status_;
    size_t length_;
    std::string type_;
    std::string body_;
    std::map<std::string, std::string> header_;
};

std::string HTTPResponse::statusToString(HTTPStatus status)
{
    std::string result;
    switch (status)
    {
    case OK:
        result = "OK";
        break;
    
    case BAD_REQUEST: 
        result = "Bad Request";
        break;

    default:
        result = "unknowStatus";
        break;
    }

    return result;
}

std::string HTTPResponse::toString()
{
    std::string resp;
    resp += version_ + " " + std::to_string(status_) + " " + statusToString(status_) + "\r\n";
    resp += "Content-Length: " + std::to_string(length_) + "\r\n";
    resp += "Content-Type: " + type_ + "\r\n";
    for (auto head : header_) {
        resp += head.first + ": " + head.second + "\r\n";
    }
    resp += "\r\n";
    resp += body_;

    return resp;
}

class HTTPCodec
{
    typedef std::function<HTTPResponse (HTTPRequest* req)> RequestCallback;
public:
    HTTPCodec(const RequestCallback& cb);
    ~HTTPCodec();

    void onMessage(const TCPConnectionPtr& conn, std::string& buffer);

private:
    RequestCallback requestCallback_;
};

HTTPCodec::HTTPCodec(const RequestCallback& cb)
    : requestCallback_(cb)
{
}

HTTPCodec::~HTTPCodec()
{
}

void HTTPCodec::onMessage(const TCPConnectionPtr& conn, std::string& buffer)
{
    HTTPRequest req(std::move(buffer));
    HTTPResponse resp = requestCallback_(&req);
    resp.setVersion(req.version());
    resp.setHeader("Connection", req.getHeader("Connection"));
    conn->send(resp.toString());
    if (req.getHeader("Connection") == "close" || resp.status() != HTTPResponse::OK)
        conn->shutdown();
}

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
    : server_(host, port, &loop_, "HTTP"),
    codec_(std::bind(&HTTPServer::onRequest, this, std::placeholders::_1))
{
    server_.setMessageCallback(std::bind(&HTTPCodec::onMessage, &codec_, std::placeholders::_1, std::placeholders::_2));
    //server_.setConnectionCallback();
}

HTTPServer::~HTTPServer()
{
}

HTTPResponse HTTPServer::onRequest(HTTPRequest* req)
{
    printf("path = %s\n", req->path().c_str());
    HTTPResponse resp;
    std::string body;

    if (req->path() == "/") {
        if (req->method() == HTTPRequest::GET) {
            body += "GET ";
        }
        body += req->version() + "</br>";
        resp.setStatus(HTTPResponse::OK);
        resp.setContentType("text/html");
        body += "<h1>Hello World</h1>";
/*
        time_t rawtime;
        struct tm *info;
        char buffer[80];

        time(&rawtime);    
        info = localtime(&rawtime);
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
        std::string now = std::string(buffer);
        body += now;
*/
        resp.setBody(body);
        resp.setContentLength(body.size());
    } else if(req->path() == "/hello") {
        resp.setStatus(HTTPResponse::OK);
        resp.setBody("hello world\n");
        resp.setContentType("text/plain");
        resp.setContentLength(12);
    } else if (req->path() == "/favicon.ico") {
        resp.setStatus(HTTPResponse::BAD_REQUEST);
    } else {
        resp.setStatus(HTTPResponse::BAD_REQUEST);
    }
    
    return resp;
}

int main(int argc, char** argv)
{
    Logger::setLogLevel(Logger::INFO);

    if (argc != 4) {
        printf("Usage: %s ip port thread\n", argv[0]);
    } else {
        HTTPServer httpServer(argv[1], static_cast<uint16_t>(atoi(argv[2])));
        httpServer.setThreadNum(static_cast<uint16_t>(atoi(argv[3])));
        httpServer.start();
    }
}