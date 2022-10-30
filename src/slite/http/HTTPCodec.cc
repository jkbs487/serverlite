#include "HTTPCodec.h"
#include "slite/Logger.h"

using namespace slite::http;

const std::string HTTPCodec::kCRLF = "\r\n";

HTTPCodec::HTTPCodec(const RequestCallback& cb)
    : requestCallback_(cb),
    state_(HTTPRequestState::kRequestLine)
{
}

HTTPCodec::~HTTPCodec()
{
}

void HTTPCodec::parseRequestLine(HTTPRequest* request, const std::string& requestLine)
{
    LOG_DEBUG << requestLine;
    size_t curPos = requestLine.find_first_of(" ");
    HTTPMethod method = requestLine.substr(0, curPos) == "GET" ? HTTPMethod::GET : HTTPMethod::POST;
    request->setMethod(method);

    size_t prevPos = curPos;
    curPos = requestLine.find_first_of(" ", prevPos + 1);
    std::string path = requestLine.substr(prevPos + 1, curPos - prevPos - 1);
    request->setPath(path);

    prevPos = requestLine.find_last_of(" ");
    std::string version = requestLine.substr(prevPos + 1, requestLine.size() - prevPos - 1);
    request->setVersion(version);
}

bool HTTPCodec::parseRequestHeader(HTTPRequest* request, const std::string& requestHeader)
{
    std::string field = requestHeader.substr(0, requestHeader.find_first_of(":"));
    if (!field.empty()) {
        std::string value = requestHeader.substr(requestHeader.find_first_of(":")+2, requestHeader.size()-field.size());
        request->addHeader(field, value);
        LOG_DEBUG << field << ": " << value;
        return false;
    }
    return true;
}

void HTTPCodec::onMessage(const TCPConnectionPtr& conn, std::string& buffer, int64_t receiveTime)
{
    HTTPRequest request = std::any_cast<HTTPRequest>(conn->getContext());
    bool isComplete = true;
    
    while (isComplete) {
        if (state_ == kRequestLine) {
            auto it = std::search(buffer.begin(), buffer.end(), kCRLF.begin(), kCRLF.end());
            if (it != buffer.end()) {
                parseRequestLine(&request, buffer.substr(0, it-buffer.begin()));
                buffer.erase(buffer.begin(), it+2);
                state_ = kRequestHeaders;
            } else {
                isComplete = false;
            }
        } else if (state_ == kRequestHeaders) {
            auto it = std::search(buffer.begin(), buffer.end(), kCRLF.begin(), kCRLF.end());
            if (it != buffer.end()) {
                bool isLast = parseRequestHeader(&request, buffer.substr(0, it-buffer.begin()));
                buffer.erase(buffer.begin(), it+2);
                if (isLast) {
                    state_ = kGotAll;
                }
            } else {
                isComplete = false;
            }
        } else if (state_ == kGotAll) {
            HTTPResponse resp;
            requestCallback_(&request, &resp);
            resp.setVersion(request.version());
            resp.setHeader("Connection", request.getHeader("Connection"));
            conn->send(resp.toString());
            if (request.getHeader("Connection") == "close")
                conn->shutdown();
            isComplete = false;
            state_ = kRequestLine;
        }
    }
}