# Problems Log

- Send error after new connection is established: Socket operation on non-socket

Epfd is incorrectly pass to TCPConnection as connfd when a new TCPConnection object created.

