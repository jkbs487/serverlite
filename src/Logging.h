#include <cstdio>
#include <mutex>

namespace slite
{

class Logging
{
public:
    Logging(std::string baseName, size_t roolSize, bool threadSave = true, 
            int flushInterval = 3, int checkEveryN = 1024);

    ~Logging() {
        if (fp_) ::fclose(fp_);
    }

    void append(const std::string& logLine);
    void flush();
private:
    std::string getLogFileName(std::string baseName, time_t* now);
    void append_unlocked(const std::string& logLine);
    void writeToFile(const std::string& logLine);
    bool rollFile();

    FILE* fp_;
    std::string baseName_;
    size_t roolSize_;
    int flushInterval_;
    int checkEveryN_;
    int count_;
    std::mutex mutex_;
    time_t lastRoll_;
    time_t lastFlush_;
    time_t startOfPeriod_;
    size_t writtenBytes_;
    bool threadSave_;

    char buffer_[64*1024];

    const static int kRollPerSeconds_ = 60*60*24;
};

} // namespace slite