#ifdef WINDOWS

#include "io/windows_named_pipe.h"

#include <iostream>

WindowsNamedPipe::WindowsNamedPipe(const std::string& path) : path(path), io() {
}

void WindowsNamedPipe::start_accepting(std::function<void(std::unique_ptr<IPCConnection>&&)> callback) {
    if(!listening) return;

    HANDLE pipe_handle = ::CreateNamedPipeA(
        path.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        0, 0, 0, nullptr
    );

    if(pipe_handle == INVALID_HANDLE_VALUE){
        throw std::runtime_error((std:: string)"Failed to create pipe " + std::to_string(GetLastError()));
    }
    
    auto event = std::make_shared<asio::windows::object_handle>(io, CreateEvent(nullptr, TRUE, FALSE, nullptr));
    if (!event->is_open()) {
        CloseHandle(pipe_handle);
        throw std::runtime_error("Failed to create windows event");
    }

    OVERLAPPED overlapped{};
    overlapped.hEvent = event->native_handle();

    bool success = ConnectNamedPipe(pipe_handle, &overlapped);
    DWORD last_error = GetLastError();

    if (!success && last_error != ERROR_IO_PENDING && last_error != ERROR_PIPE_CONNECTED) {
        CloseHandle(pipe_handle);
        throw std::runtime_error("Connecting named pipe failed");
    }

    event->async_wait(
        [this, pipe_handle, event, callback](const asio::error_code& ec) {
            if (!ec) {
                asio::windows::stream_handle stream(io, pipe_handle);
                auto connection = std::make_unique<WindowsNamedPipeConnection>(std::move(stream));
                callback(std::move(connection));

                if (listening) {
                    start_accepting(callback);
                }
            } else if (ec == asio::error::operation_aborted) {
                std::cerr << "Aborted\n";
                CloseHandle(pipe_handle);
            } else {
                CloseHandle(pipe_handle);
                throw std::runtime_error("Encountered an error while connecting to a client: " + ec.message());
            }
        }
    );
}


void WindowsNamedPipe::listen(std::function<void(std::unique_ptr<IPCConnection>&&)> callback) {
    listening = true;

    start_accepting(callback);
    
    io.run();
} 

void WindowsNamedPipe::stop(){
    listening = false;
    io.stop();
}

WindowsNamedPipeConnection::WindowsNamedPipeConnection(asio::windows::stream_handle pipe)
        : pipe(std::move(pipe)) {
}

void WindowsNamedPipeConnection::send(const std::string& message) {
    asio::write(pipe, asio::buffer(message + "\n"));
}

std::string WindowsNamedPipeConnection::receive() {
    asio::streambuf buffer;
    asio::read_until(pipe, buffer, '\n');
    return std::string(asio::buffers_begin(buffer.data()), asio::buffers_begin(buffer.data()) + buffer.size() -  2); //-2 to trim newline
}

#endif