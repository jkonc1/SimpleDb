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
    
    void add_job(std::move_only_function<void()> task);
    void finish();
private:
    std::mutex mutex;
    std::queue<std::thread> running_workers;
};

#endif