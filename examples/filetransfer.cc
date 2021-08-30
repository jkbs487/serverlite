#include "TCPServer.h"
#include "EventLoop.h"

#include <ctime>
#include <cstdio>
/*
std::string readFile(const char *filepath)
{
    FILE* pFile = ::fopen(filepath, "rb");
    if (!pFile) perror("fopen");

    fseek (pFile ,0 , SEEK_END);
    long fileSize = ftell(pFile);
    rewind (pFile);
    printf("filename: %s, filesize: %ld\n", filepath, fileSize);

    char buffer[1024*1024];
    bzero(buffer, sizeof buffer);
    size_t nread = 0;
    std::string fileContent;
    while ((nread = ::fread(buffer, 1, sizeof buffer, pFile)) > 0) {
        fileContent.append(buffer, nread);
    }
    return fileContent;
}
*/
void onConnection(const TCPConnectionPtr& conn)
{
    if (conn->connected()) {
        printf("UP\n");
    }
    else {
        printf("DOWN\n");
    }
}

void onMessage(const TCPConnectionPtr& conn, std::string buffer)
{
    if (buffer == "recv") {
        conn->send("start transfer...");
        conn->sendFile("/home/jiangkun/test/client.c");
    }
    if (buffer == "exit") {
        conn->shutdown();
    }
}

int main(int argc, char *argv[])
{
    EventLoop eventLoop;
    TCPServer tcpserver("0.0.0.0", 12345, &eventLoop);
    tcpserver.setConnectionCallback(onConnection);
    tcpserver.setMessageCallback(onMessage);
    tcpserver.start();
    eventLoop.loop();
    return 0;
}