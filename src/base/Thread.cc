#include "src/base/Thread.h"

#include <assert.h>
#include <cstring>
#include <semaphore.h>

using namespace mymuduo;

std::atomic_int32_t Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false),        // 还未开始
      joined_(false),         // 还未设置等待线程
      threadId_(0),                // 初始 tid 设置为0
      func_(std::move(func)), // EventLoopThread::threadFunc()
      name_(name)             // 默认名称是空字符串""

{
  // 设置线程索引编号和姓名
  int num = ++numCreated_;
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Thread%d", num);
    name_ = buf;
  }
}

Thread::~Thread() {
  // 现场感启动时并且未设置等待线程时才可调用
  if (started_ && !joined_) {
    // 设置线程分离(守护线程，不需要等待线程结束，不会产生孤儿线程)
    thread_->detach();
  }
}

void Thread::start() {
  assert(!started_);
  sem_t sem;
  sem_init(&sem, false, 0);
  started_ = true;
  // 开启线程
  thread_ = std::make_shared<std::thread>([&]() {
    threadId_ = std::this_thread::get_id();
    sem_post(&sem);
    func_();
  });
  /**
   * 这里必须等待获取上面新创建的线程的tid
   * 未获取到信息则不会执行sem_post，所以会被阻塞住
   * 如果不使用信号量操作，则别的线程访问tid时候，可能上面的线程还没有获取到tid
   */
  sem_wait(&sem);
}

void Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  // 等待线程执行完毕
  thread_->join();
}

uint64_t Thread::threadId() const {
  uint64_t tid;
  memcpy(&tid, &threadId_, 8);
  return tid;
}