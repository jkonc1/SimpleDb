#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include <thread>
#include <functional>
#include <queue>
#include <mutex>

/**
 * @brief A queue that manages background jobs in separate threads
 */
class JobQueue {
public:
    JobQueue();
    
    ~JobQueue();
    
    JobQueue(const JobQueue&) = delete;
    
    JobQueue& operator=(const JobQueue&) = delete;
    
    /**
     * @brief Add a new job to the queue
     * @param task The function to execute in a new thread (must be noexcept)
     * @details Creates a new thread to execute the task
     */
    void add_job(std::move_only_function<void() noexcept> task);
    
    /**
     * @brief Wait for all jobs to complete
     */
    void finish();
private:
    std::mutex mutex;
    std::queue<std::thread> running_workers;
};

#endif