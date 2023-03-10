#include "src/net/EventLoopThread.h"
#include "src/net/EventLoop.h"

using namespace mymuduo;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr), exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this),
              name), // 新线程绑定此函数
      callback_(cb) // 传入的线程初始化回调函数，用户自定义的
{}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (loop_ != nullptr) {
    loop_->quit();
    thread_.join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  thread_.start();

  EventLoop *loop = nullptr;
  {
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

  // 用户自定义的函数
  if (callback_) {
    callback_(&loop);
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop; // 等到生成EventLoop对象之后才唤醒
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