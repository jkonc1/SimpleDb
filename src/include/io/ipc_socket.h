#ifndef IPC_SOCKET_H
#define IPC_SOCKET_H

#include <string>
#include <memory>

class IPCConnection{
public:
    virtual void send(const std::string& message) = 0;
    virtual std::string receive() = 0;
    
    IPCConnection() = default;
    IPCConnection(const IPCConnection&) = delete;
    IPCConnection& operator=(const IPCConnection&) = delete;
};

class IPCSocket {
public:
    virtual std::unique_ptr<IPCConnection> accept() = 0;
    
    IPCSocket() = default;
    IPCSocket(const IPCSocket&) = delete;
    IPCSocket& operator=(const IPCSocket&) = delete;
};

#endif