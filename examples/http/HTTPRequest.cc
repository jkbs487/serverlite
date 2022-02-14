#include "HTTPRequest.h"

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