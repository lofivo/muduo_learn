#ifndef MYMUDUO_LOGGER_LOGFILE_H
#define MYMUDUO_LOGGER_LOGFILE_H

#include "FileUtil.h"

#include <memory>
#include <mutex>

namespace mymuduo {
class LogFile {
public:
  LogFile(const std::string &basename, off_t rollSize, bool threadSafe = true,
          int flushInterval = 3, int checkEveryN = 1024);
  ~LogFile();

  void append(const char *data, int len);
  void flush();
  bool rollFile(); // 滚动日志

private:
  static std::string getLogFileName(const std::string &basename, time_t *now);
  void appendUnLocked(const char *data, int len);

  const std::string basename_;
  const off_t rollSize_;    // 表示一次最大刷新的字节数
  const int flushInterval_; // 刷新的时间间隔，单位为毫秒
  const int checkEveryN_;   // 表示写数据的次数限制，默认为 1024

  int count_; // 用来计数当前写数据的次数,如果超过 checkEveryN 则清除以重新计数

  std::unique_ptr<std::mutex> mutex_;
  time_t startOfPeriod_; // 本次写日志的起始时间
  time_t lastRoll_;      // 上次滚动日志文件的时间
  time_t lastFlush_;     // 上次刷新日志文件的时间
  std::unique_ptr<FileUtil> file_;

  const static int kRollPerSeconds_ = 60 * 60 * 24;
};
} // namespace mymuduo

#endif // MYMUDUO_LOGGER_LOGFILE_H