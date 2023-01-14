#include "LogFile.h"

using namespace mymuduo;

LogFile::LogFile(const std::string &basename, off_t rollSize, bool threadSafe,
                 int flushInterval, int checkEveryN)
    : basename_(basename), rollSize_(rollSize), flushInterval_(flushInterval),
      checkEveryN_(checkEveryN), count_(0),
      mutex_(threadSafe ? new std::mutex : nullptr), startOfPeriod_(0),
      lastRoll_(0), lastFlush_(0) {
  rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char *data, int len) {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    appendUnLocked(data, len);
  } else {
    appendUnLocked(data, len);
  }
}

void LogFile::appendUnLocked(const char *data, int len) {
  file_->append(data, len);
  // 如果日志文件已经写入的字节数大于规定的滚动大小,就要滚动日志文件
  if (file_->writtenBytes() > rollSize_) {
    rollFile();
  } else {
    ++count_;
    // 如果写入数据的次数达到次数上限
    if (count_ >= checkEveryN_) {
      count_ = 0;
      time_t now = ::time(NULL);
      // 这次写到上限的时间
      time_t thisPeriod = now / kRollPerSeconds_ * kRollPerSeconds_;
      // 确保该次写到上限的时间比起始写日志的时间大于1秒
      // 避免频繁滚动日志文件
      if (thisPeriod != startOfPeriod_) {
        rollFile();
      } else if (now - lastFlush_ > flushInterval_) {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

void LogFile::flush() {
  if (mutex_) {
    std::lock_guard<std::mutex> lock(*mutex_);
    file_->flush();
  } else {
    file_->flush();
  }
}

// 滚动日志
bool LogFile::rollFile() {
  time_t now = 0;
  // getLogFileName会修改now的值使其等于现在的时间
  std::string filename = getLogFileName(basename_, &now);
  // now/kRollPerSeconds求出现在是第几天，再乘以秒数相当于是当前天数0点对应的秒数
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
  // 为避免频繁创建新文件,上次滚动事件到现在超过1秒才会发生滚动
  if (now > lastRoll_) {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    // 让file_指向一个名为filename的文件，相当于新建了一个文件
    file_.reset(new FileUtil(filename));
    return true;
  }
  return false;
}

// [basename].[nowtime].[hostname].[pid].log
std::string LogFile::getLogFileName(const std::string &basename, time_t *now) {
  std::string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  localtime_r(now, &tm);
  // 写入时间
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", &tm);
  filename += timebuf;

  filename += ".log";

  return filename;
}