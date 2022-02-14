#include "HTTPCodec.h"

HTTPCodec::HTTPCodec(const RequestCallback& cb)
    : requestCallback_(cb)
{
}

HTTPCodec::~HTTPCodec()
{
}

void HTTPCodec::onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime)
{
    HTTPRequest req(std::move(buffer));
    HTTPResponse resp = requestCallback_(&req);
    resp.setVersion(req.version());
    resp.setHeader("Connection", req.getHeader("Connection"));
    conn->send(resp.toString());
    if (req.getHeader("Connection") == "close" || resp.status() != HTTPResponse::OK)
        conn->shutdown();
}