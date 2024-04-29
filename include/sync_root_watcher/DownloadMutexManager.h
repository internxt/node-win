#include <mutex>
#include <condition_variable>
#include "Logger.h"
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
        std::unique_lock<std::mutex> lock(downloadMtx);
        while (!downloadReady)
        {
            Logger::getInstance().log("Waiting...", LogLevel::INFO);
            downloadCv.wait(lock);
        }
    }

    void setReady(bool loadFinished)
    {
        if (loadFinished)
        {
            std::lock_guard<std::mutex> lock(downloadMtx);
            downloadReady = true;
            downloadCv.notify_one();
        }
    }
    void resetReady()
    {
        std::lock_guard<std::mutex> lock(downloadMtx);
        downloadReady = false;
    }

private:
    std::mutex downloadMtx;
    std::condition_variable downloadCv;
    bool downloadReady = false;

    // Constructor privado
    DownloadMutexManager() = default;
};
