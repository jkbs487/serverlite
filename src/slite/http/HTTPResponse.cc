#include "HTTPResponse.h"

using namespace slite::http;

std::string HTTPResponse::statusToString(HTTPStatus status)
{
    std::string result;
    switch (status)
    {
    case OK:
        result = "OK";
        break;
    
    case BAD_REQUEST: 
        result = "Bad Request";
        break;

    case NOT_FOUND: 
        result = "Not Found";
        break;

    default:
        result = "unknowStatus";
        break;
    }

    return result;
}

std::string HTTPResponse::toString()
{
    std::string resp;
    resp += version_ + " " + std::to_string(status_) + " " + statusToString(status_) + "\r\n";
    if (length_) resp += "Content-Length: " + std::to_string(length_) + "\r\n";
    if (!type_.empty()) resp += "Content-Type: " + type_ + "\r\n";
    for (auto head : header_) {
        resp += head.first + ": " + head.second + "\r\n";
    }
    resp += "\r\n";
    resp += body_;

    return resp;
}
