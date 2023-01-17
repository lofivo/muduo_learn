#ifndef MYMUDUO_BASE_THREAD_H
#define MYMUDUO_BASE_THREAD_H

#include "src/base/noncopyable.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace mymuduo {

class Thread : noncopyable {
public:
  using ThreadFunc = std::function<void()>;
public:
  explicit Thread(ThreadFunc, const std::string &name = std::string());
  ~Thread();

  void start(); // 开启线程
  void join();  // 等待线程

  bool started() const { return started_; }
  unsigned long tid() const;
  const std::string &name() const { return name_; }

  static int numCreated() { return numCreated_; }

private:
  bool started_; // 是否启动线程
  bool joined_;  // 是否等待该线程
  std::shared_ptr<std::thread> thread_;
  std::thread::id tid_;
  // Thread::start() 调用的回调函数
  // 其实保存的是 EventLoopThread::threadFunc()
  ThreadFunc func_;
  std::string name_;                      // 线程名
  static std::atomic_int32_t numCreated_; // 线程索引
};

} // namespace mymuduo

#endif // MYMUDUO_BASE_THREAD_H