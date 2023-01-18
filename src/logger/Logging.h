#ifndef MYMUDUO_LOGGER_LOGGING_H
#define MYMUDUO_LOGGER_LOGGING_H

#include "src/base/Timestamp.h"
#include "src/logger/LogStream.h"

#include <errno.h>
#include <functional>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

namespace mymuduo {
// SourceFile的作用是提取文件名
class SourceFile {
public:
  explicit SourceFile(const char *filename) : data_(filename) {
    /**
     * 找出data中出现/最后一次的位置，从而获取具体的文件名
     * 2022/10/26/test.log
     */
    const char *slash = strrchr(filename, '/');
    if (slash) {
      data_ = slash + 1;
    }
    size_ = static_cast<int>(strlen(data_));
  }

  const char *data_;
  int size_;
};

class Logger {
public:
  enum LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    LEVEL_COUNT,
  };

  // member function
  Logger(const char *file, int line);
  Logger(const char *file, int line, LogLevel level);
  Logger(const char *file, int line, LogLevel level, const char *func);
  Logger(const char *file, int line, bool toAbort);
  ~Logger();

  // 流是会改变的
  LogStream &stream() { return impl_.stream_; }

  // TODO:static关键字作用的函数必须在源文件实现?
  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  // 输出函数和刷新缓冲区函数
  using OutputFunc = std::function<void(const char *msg, int len)>;
  using FlushFunc = std::function<void()>;
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);

private:
  // 内部类
  class Impl {
  public:
    using LogLevel = Logger::LogLevel;
    Impl(LogLevel level, int savedErrno, const char *file, int line);
    void formatTime();
    void finish();

    Timestamp time_;
    LogStream stream_;
    LogLevel level_;
    int line_;
    SourceFile basename_;
  };

  // Logger's member variable
  Impl impl_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel logLevel() { return g_logLevel; }

// 获取errno信息
const char *getErrnoMsg(int savedErrno);

/**
 * 当日志等级小于对应等级才会输出
 * 比如设置等级为FATAL，则logLevel等级大于DEBUG和INFO，DEBUG和INFO等级的日志就不会输出
 */
#define LOG_TRACE                                                              \
  if (logLevel() <= Logger::TRACE)                                             \
  Logger(__FILE__, __LINE__, Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                                              \
  if (logLevel() <= Logger::DEBUG)                                             \
  Logger(__FILE__, __LINE__, Logger::DEBUG, __func__).stream()
#define LOG_INFO                                                               \
  if (logLevel() <= Logger::INFO)                                              \
  Logger(__FILE__, __LINE__).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#define LOG_SYSERR Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL Logger(__FILE__, __LINE__, true).stream()

} // namespace mymuduo

#endif // MYMUDUO_LOGGER_LOGGING_H