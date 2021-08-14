#include "TCPServer.h"

#include <ctime>
#include <cstdio>

void echo(TCPConnection *conn, std::string buffer)
{
    printf("echo %ld bytes\n", buffer.size());

    conn->send(buffer);
}

void discard(TCPConnection *conn, std::string buffer)
{
    printf("discards %ld bytes\n", buffer.size());
}

void daytime(TCPConnection *conn)
{
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);    
    info = localtime(&rawtime);
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
    if (conn->connected()) {
        conn->send(std::string(buffer));
        conn->shutdown();
    }
}

void time_(TCPConnection *conn)
{
    time_t now = ::time(NULL);

    int32_t be32 = htobe32(static_cast<int32_t>(now));
    if (conn->connected()) {
        conn->send(std::to_string(be32));
        conn->shutdown();
    }
}

std::string message;

void chargen(TCPConnection *conn)
{
    if (conn->connected()) {
        conn->send(message);
    }
}

void onWriteComplete(TCPConnection *conn)
{
    if (conn->connected()) {
        conn->send(message);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s [service]\n", argv[0]);
        ::exit(1);
    }
/*
    std::string line;
    for (int i = 33; i < 127; ++i)
    {
      line.push_back(char(i));
    }
    line += line;

    for (size_t i = 0; i < 127-33; ++i)
    {
      message += line.substr(i, 72) + '\n';
    }
*/

    TCPServer tcpserver("0.0.0.0", 12345);

    if (!strcmp(argv[1], "discard")) {
        tcpserver.setMessageCallback(discard);
    } else if (!strcmp(argv[1], "daytime")) {
        tcpserver.setConnectionCallback(daytime);
    } else if (!strcmp(argv[1], "time")) {
        tcpserver.setConnectionCallback(time_);
    } else if (!strcmp(argv[1], "echo")) {
        tcpserver.setMessageCallback(echo);
    } else if (!strcmp(argv[1], "chargen")) {
        tcpserver.setConnectionCallback(chargen);
        tcpserver.setMessageCallback(discard);
        tcpserver.setWriteCompleteCallback(onWriteComplete);    
    } else {
        printf("server error! [discard daytime time echo chargen]\n");
        ::exit(1);
    }

    tcpserver.start();
    return 0;
}