#include "jobs/job_queue.h"

#include <cassert>

JobQueue::JobQueue() {
}

JobQueue::~JobQueue() {
    assert(running_workers.empty());
}

void JobQueue::finish() {
    auto lock = std::unique_lock(mutex);
    while (!running_workers.empty()) {
        running_workers.front().join();
        running_workers.pop();
    }
}

void JobQueue::add_job(std::move_only_function<void() noexcept> task){
    auto lock = std::unique_lock(mutex);
    
    running_workers.push(std::thread([task = std::move(task)]() mutable {
        task();
    }));
}
