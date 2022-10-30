#pragma once

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "slite/TCPConnection.h"

#include <functional>

namespace slite
{
namespace http
{

class HTTPCodec
{
    using RequestCallback = std::function<void (HTTPRequest* req, HTTPResponse* resp)>;
public:
    enum HTTPRequestState {
        kRequestLine,
        kRequestHeaders,
        kRequestBody,
        kGotAll
    };

    HTTPCodec(const RequestCallback& cb);
    ~HTTPCodec();

    void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);

private:
    void parseRequestLine(HTTPRequest* request, const std::string& requestLine);
    bool parseRequestHeader(HTTPRequest* request, const std::string& requestHeader);

    RequestCallback requestCallback_;
    HTTPRequestState state_;
    static const std::string kCRLF;
};

} // namespace http
} // namespace slite