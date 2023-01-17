#ifndef MYMUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MYMUDUO_NET_EVENTLOOPTHREADPOOL_H

#include "src/base/noncopyable.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mymuduo {
class EventLoop;
class EventLoopThread;
class EventLoopThreadPool : noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

public:
  EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
  ~EventLoopThreadPool();
  // 设置线程数量
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }

  // 启动线程池
  void start(const ThreadInitCallback &cb = ThreadInitCallback());

  // 如果工作在多线程中，baseLoop_(mainLoop)会默认以轮询的方式分配Channel给subLoop
  EventLoop *getNextLoop();

  std::vector<EventLoop *> getAllLoops();

  bool started() const { return started_; }
  const std::string& name() const { return name_; }

private:
  EventLoop *baseLoop_; // 用户使用muduo创建的loop 如果线程数为1
                        // 那直接使用用户创建的loop 否则创建多EventLoop
  std::string name_;
  bool started_;   // 开启线程池标志
  int numThreads_; // 创建线程数量
  int next_;       // 轮询的下标
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_EVENTLOOPTHREADPOOL_H