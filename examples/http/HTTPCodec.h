#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "TCPConnection.h"

#include <functional>

using namespace slite;

class HTTPCodec
{
    typedef std::function<HTTPResponse (HTTPRequest* req)> RequestCallback;
public:
    HTTPCodec(const RequestCallback& cb);
    ~HTTPCodec();

    void onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime);

private:
    RequestCallback requestCallback_;
};