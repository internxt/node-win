#include "Semaphore.h"

Semaphore::Semaphore(int count_)
    : count(count_) {}

void Semaphore::release() {
    std::unique_lock<std::mutex> lock(mtx);
    count++;
    cv.notify_one();
}

void Semaphore::acquire() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&] { return count > 0; });
    count--;
}
