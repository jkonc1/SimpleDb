#ifndef IPC_SOCKET_H
#define IPC_SOCKET_H

#include <string>
#include <memory>
#include <functional>

/**
 * @brief Abstract base class representing a IPC connection
 */
class IPCConnection{
public:
    /**
     * @brief Send a message
     * @param message The string message to send 
     */
    virtual void send(const std::string& message) = 0;
    
    /**
     * @brief Receive a message (line) from the IPC connection
     */
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
    
    /**
     * @brief Block and start listening for connections and process them with a callback
     */
    virtual void listen(std::function<void(std::unique_ptr<IPCConnection>&&)> callback) = 0;
    
    /**
     * @brief Stop listening
     */
    virtual void stop() = 0;
    
    virtual ~IPCSocket() = default;
};

#endif