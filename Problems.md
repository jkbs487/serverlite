# Problems Log

- Send error after new connection is established: Socket operation on non-socket

    Epfd is incorrectly pass to TCPConnection as connfd when a new TCPConnection object created.

- Do not reregister None Event Channel to Poller, this will cause busy loop

    Epoll cannot register None Event, need to set the state member in the channel, add Event when update channel 

- Why sepearate EventLoop and TcpServer?

    Can make multiple TCPServers reuse one EventLoop.

- Chargen Server Segmentation fault, when Client Interrupt with Ctrl-C
    
    Channel has been destroy by Read Event (read eof, close connnection), cause to use the wild pointer (sendCallback).
    Channel maybe destroy when handleEvent, so need extend the life of the Channel. 