#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "slite/TCPConnection.h"

#include <functional>

using namespace slite;

class HTTPCodec
{
    typedef std::function<HTTPResponse (HTTPRequest* req)> RequestCallback;
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
    void parseRequestLine(HTTPRequest* request, std::string requestLine);
    bool parseRequestHeader(HTTPRequest* request, std::string requestHeader);

    RequestCallback requestCallback_;
    HTTPRequestState state_;
    static const std::string kCRLF;
};