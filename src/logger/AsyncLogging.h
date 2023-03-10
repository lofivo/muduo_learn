#ifndef MYMUDUO_LOGGER_ASYNCLOGGING_H
#define MYMUDUO_LOGGER_ASYNCLOGGING_H


#include "src/logger/FixedBuffer.h"
#include "src/logger/LogFile.h"
#include "src/logger/LogStream.h"
#include "src/base/Thread.h"
#include "src/base/noncopyable.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace mymuduo {
class AsyncLogging {
public:
  AsyncLogging(const std::string &basename, off_t rollSize,
               int flushInterval = 3);
  ~AsyncLogging() {
    if (running_) {
      stop();
    }
  }

  // 前端调用 append 写入日志
  void append(const char *logling, int len);

  void start() {
    running_ = true;
    thread_.start();
  }

  void stop() {
    running_ = false;
    cond_.notify_one();
    thread_.join();
  }

private:
  using Buffer = FixedBuffer<kLargeBuffer>;
  using BufferVector = std::vector<std::unique_ptr<Buffer>>;
  using BufferPtr = BufferVector::value_type;

  void threadFunc();

  const int flushInterval_;
  std::atomic<bool> running_;
  const std::string basename_;
  const off_t rollSize_;
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;

  BufferPtr currentBuffer_;
  BufferPtr nextBuffer_;
  BufferVector buffers_;
};
} // namespace mymuduo

#endif // MYMUDUO_LOGGER_ASYNCLOGGING_H_