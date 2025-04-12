#ifndef IPC_SOCKET_H
#define IPC_SOCKET_H

#include <string>
#include <memory>
#include <functional>

class IPCConnection{
public:
    virtual void send(const std::string& message) = 0;
    virtual std::string receive() = 0;
    
    IPCConnection() = default;
    IPCConnection(const IPCConnection&) = delete;
    IPCConnection& operator=(const IPCConnection&) = delete;
    
    virtual ~IPCConnection() = default;
};

class IPCSocket {
public:
    IPCSocket() = default;
    IPCSocket(const IPCSocket&) = delete;
    IPCSocket& operator=(const IPCSocket&) = delete;
    virtual void listen(std::function<void(std::unique_ptr<IPCConnection>&&)> callback) = 0;
    virtual void stop() = 0;
    
    virtual ~IPCSocket() = default;
};

#endif