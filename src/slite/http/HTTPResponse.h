#include <string>
#include <map>

class HTTPResponse
{
public:
    enum HTTPStatus {
        OK = 200,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
        SERVER_ERROR = 500
    };

    HTTPResponse():
        version_("HTTP/1.1")
    {};
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