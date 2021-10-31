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

- When use EventLoopThread to start a TCPServer: "read eventfd: Resource temporarily unavailable"

    Eventfd do not initialized with the initialization list of EventLoop, so .

- Cannot assign requested address

    Insufficient ports.

- The more connections, the slower and slower the connection speed

    Single thread, full-connection queue is full, take connection speed is too slow.

- terminate called without an active exception

    std::thread do not use thread.join method.

- How to know std::thread can join ?

    use thread.joinable method.

- In TCPConnection::handleClose, why use guardThis(shared_from_this()) instead of guardThis(this)?

    Only use "shared_from_this" will add current TCPConnectionPtr use count, use "this" means construct a new shared_ptr