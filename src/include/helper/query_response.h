#ifndef QUERY_RESPONSE_H
#define QUERY_RESPONSE_H

#include <string>

struct QueryResponse {
    enum class Status {
        SUCCESS,
        ERROR
    };

    Status status;
    std::string message;
    
    QueryResponse(Status status, std::string message) : status(status), message(std::move(message)) {}
    
    std::string to_string() const {
        return std::string(status == Status::SUCCESS ? "OK" : "ERROR") + " " + message;
    }
};

#endif