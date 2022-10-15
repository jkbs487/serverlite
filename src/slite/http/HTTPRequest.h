#pragma once

#include <string>
#include <map>

namespace slite
{
namespace http
{

enum HTTPMethod {
    GET,
    POST
};

class HTTPRequest
{
public:
    HTTPRequest();
    ~HTTPRequest() {};

    HTTPMethod method() 
    { return method_; }

    std::string path()
    { return path_; }

    std::string version()
    { return version_; }

    void setMethod(HTTPMethod method) { method_ = method; }
    void setPath(std::string path) { path_ = path; }
    void setVersion(std::string version) { version_ = version; }
    std::string getHeader(std::string field);
    void addHeader(std::string field, std::string value);

private:
    void parseHttpRequest(std::string content);

    HTTPMethod method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> header_;
};

}
}