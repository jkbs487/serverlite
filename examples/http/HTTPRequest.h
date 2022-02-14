#include <string>
#include <map>

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