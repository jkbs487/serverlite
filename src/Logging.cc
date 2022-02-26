#include "Logging.h"

#include <cstring>
#include <cassert>
#include <string>
#include <unistd.h>

using namespace slite;

Logging::Logging(std::string baseName, size_t roolSize, bool threadSave, 
                int flushInterval, int checkEveryN)
    : fp_(nullptr),
    baseName_(baseName),
    roolSize_(roolSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    lastRoll_(0),
    lastFlush_(0),
    startOfPeriod_(0),
    threadSave_(threadSave)
{
    //assert(basename.find('/') == std::string::npos);
    rollFile();
}

void Logging::append(const std::string& logLine)
{
    if (threadSave_) {
        std::lock_guard<std::mutex> lock(mutex_);
        append_unlocked(logLine);
    } else {
        append_unlocked(logLine);
    }
}

void Logging::append_unlocked(const std::string& logLine)
{   
    writeToFile(logLine);
    if (writtenBytes_ > roolSize_) {
        rollFile();
    } else {
        ++count_;
        if (count_ >= checkEveryN_) {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (thisPeriod != startOfPeriod_) {
                rollFile();
            } else if (now - lastFlush_ > flushInterval_) {
                lastFlush_ = now;
                ::fflush(fp_);
            }
        }
    }
}

void Logging::writeToFile(const std::string& logLine)
{
    size_t written = 0;
    while (written != logLine.size())
    {
        size_t remain = logLine.size() - written;
        size_t n = ::fwrite_unlocked(logLine.c_str(), 1, logLine.size(), fp_);
        if (n != remain) {
            int err = ferror(fp_);
            if (err) {
                fprintf(stderr, "Logging::append() failed %s\n", strerror(err));
                break;
            }
        }
        written += n;
    }
    writtenBytes_ += written;
}

std::string Logging::getLogFileName(std::string baseName, time_t* now)
{
    std::string fileName;
    fileName.reserve(baseName.size() + 64);
    fileName = baseName;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    gmtime_r(now, &tm);

    // add time
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    fileName += timebuf;
    
    // add hostname
    char buf[256];
    if (::gethostname(buf, sizeof buf) == 0) {
        buf[sizeof(buf)-1] = '\0';
        fileName += std::string(buf);
    } else {
        fileName += "unknownhost";
    }

    // add pid
    char pidbuf[32];
    snprintf(pidbuf, sizeof pidbuf, ".%d", ::getpid());
    fileName += pidbuf;

    fileName += ".log";

    return fileName;
}

void Logging::flush()
{
    if (threadSave_) {
        std::lock_guard<std::mutex> lock(mutex_);
        ::fflush(fp_);
    } else {
        ::fflush(fp_);
    }
}

bool Logging::rollFile()
{
    time_t now = 0;
    std::string filename = getLogFileName(baseName_, &now);
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_) {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        writtenBytes_ = 0;
        
        if (fp_) ::fclose(fp_);
        fp_ = ::fopen(filename.c_str(), "ae");
        ::setbuffer(fp_, buffer_, sizeof buffer_);
        if (!fp_) {
            perror("fopen");
            abort();
        }
        return true;
    }
    return false;   
}