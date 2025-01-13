#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
#include <condition_variable>

class Semaphore {
public:
    explicit Semaphore(int count_ = 0);
    void release();
    void acquire();

private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
};

#endif
