#include "src/net/EventLoopThread.h"
#include "src/net/EventLoop.h"

#include <assert.h>
using namespace mymuduo;

EventLoopThread::EventLoopThread()
    : loop_(nullptr), exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this)), mutex_(),
      cond_() {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (loop_ != nullptr) {
    loop_->quit();
    thread_.join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  assert(!thread_.started());
  thread_.start();
  EventLoop *loop = nullptr;
  {
    // 等待新线程执行threadFunc完毕，所以使用cond_.wait
    std::unique_lock<std::mutex> lock(mutex_);
    while (loop_ == nullptr) {
      cond_.wait(lock);
    }
    loop = loop_;
  }
  return loop_;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;

  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }
  // 执行EventLoop的loop() 开启了底层的Poller的poll()
  // 这个是subLoop
  loop.loop();
  // loop是一个事件循环，如果往下执行说明停止了事件循环，需要关闭eventLoop
  // 此处是获取互斥锁再置loop_为空
  std::unique_lock<std::mutex> lock(mutex_);
  loop_ = nullptr;
}
