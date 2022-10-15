#include "HTTPRequest.h"

using namespace slite::http;

HTTPRequest::HTTPRequest()
{
}

std::string HTTPRequest::getHeader(std::string field)
{
    return header_[field];
}

void HTTPRequest::addHeader(std::string field, std::string value)
{
    header_[field] = value;
}