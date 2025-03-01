#ifndef IPC_SOCKET_H
#define IPC_SOCKET_H

#include <string>
#include <memory>
#include <functional>

class IPCConnection{
public:
    virtual void send(const std::string& message) = 0;
    virtual std::string receive() = 0;
    
    virtual ~IPCConnection() = default;
};

class IPCSocket {
public:
    virtual void listen(std::function<void(std::unique_ptr<IPCConnection>&&)> callback) = 0;
    virtual void stop() = 0;
    
    virtual ~IPCSocket() = default;
};

#endif