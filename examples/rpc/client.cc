#include "examples/rpc/sudoku.pb.h"

#include "slite/Logger.h"
#include "slite/protorpc/RpcChannel.h"
#include <unistd.h>

void solved(sudoku::SudokuResponse* resp)
{
    LOG_INFO << "solved:\n" << resp->DebugString();
}

int main() {
    // slite::Logger::setLogLevel(slite::Logger::DEBUG);
    slite::RpcChannel channel;
    channel.setServer("127.0.0.1", 9981);
    sudoku::SudokuService::Stub stub_(&channel);
    sudoku::SudokuRequest request;
    request.set_checkerboard("001010");
    sudoku::SudokuResponse* response = new sudoku::SudokuResponse;
    stub_.Solve(NULL, &request, response, google::protobuf::NewCallback(solved, response));
}