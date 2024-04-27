#include "examples/rpc/sudoku.pb.h"

#include "slite/Logger.h"
#include "slite/EventLoop.h"
#include "slite/protorpc/RpcServer.h"

#include <unistd.h>

using namespace slite;

namespace sudoku
{

class SudokuServiceImpl : public SudokuService
{
 public:
  virtual void Solve(::google::protobuf::RpcController* controller,
                       const ::sudoku::SudokuRequest* request,
                       ::sudoku::SudokuResponse* response,
                       ::google::protobuf::Closure* done)
  {
    LOG_INFO << "SudokuServiceImpl::Solve";
    response->set_solved(true);
    response->set_checkerboard("1234567");
    done->Run();
  }
};

}  // namespace sudoku

int main()
{
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  sudoku::SudokuServiceImpl impl;
  RpcServer server(&loop, "127.0.0.1", 9981);
  server.registerService(&impl);
  server.start();
  loop.loop();
  google::protobuf::ShutdownProtobufLibrary();
}