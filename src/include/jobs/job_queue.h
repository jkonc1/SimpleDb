#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include <thread>
#include <functional>
#include <queue>
#include <mutex>

class JobQueue {
public:
    JobQueue();
    ~JobQueue();
    
    JobQueue(const JobQueue&) = delete;
    JobQueue& operator=(const JobQueue&) = delete;
    
    void add_job(std::move_only_function<void() noexcept> task);
    void finish();
private:
    std::mutex mutex;
    std::queue<std::thread> running_workers;
};

#endif