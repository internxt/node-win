#include <mutex>
#include <condition_variable>

class DownloadMutexManager
{
public:
    // Elimina las maneras de copiar y asignar
    DownloadMutexManager(const DownloadMutexManager &) = delete;
    DownloadMutexManager &operator=(const DownloadMutexManager &) = delete;

    static DownloadMutexManager &getInstance()
    {
        static DownloadMutexManager instance; // Instancia única
        return instance;
    }

    void waitReady()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]()
                { return ready; });
        }

    void setReady(bool loadFinished)
    {
        if (loadFinished)
        {
            std::lock_guard<std::mutex> lock(mtx);
            ready = true;
            cv.notify_one();
        }
    }
    void resetReady()
    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = false;
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    // Constructor privado
    DownloadMutexManager() = default;
};
