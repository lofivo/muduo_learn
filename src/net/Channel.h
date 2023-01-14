#ifndef MYMUDUO_NET_CHANNEL_H
#define MYMUDUO_NET_CHANNEL_H

#include <functional>
#include <memory>

#include "src/base/Timestamp.h"
#include "src/base/noncopyable.h"
#include "src/logger/Logging.h"

namespace mymuduo {
class EventLoop;

class Channel : noncopyable {
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

  Channel(EventLoop *loop, int fd);


  void handleEvent(Timestamp receiveTime);
  void setReadCallback(const ReadEventCallback cb) {
    readCallback_ = std::move(cb);
  }
  void setWriteCallback(const EventCallback cb) {
    writeCallback_ = std::move(cb);
  }
  void setErrorCallback(const EventCallback cb) {
    errorCallback_ = std::move(cb);
  }

  // 返回封装的fd
  int fd() const { return fd_; }

  // 返回感兴趣的事件
  int events() const { return events_; }

  // 设置Poller返回的发生事件
  void set_revents(int revt) { revents_ = revt; }

  // 设置fd相应的事件状态，update()其本质调用epoll_ctl
  void enableReading() {
    events_ |= kReadEvent;
    update();
  }
  void disableReading() {
    events_ &= ~kReadEvent;
    update();
  }
  void enableWriting() {
    events_ |= kWriteEvent;
    update();
  }
  void disableWriting() {
    events_ &= ~kWriteEvent;
    update();
  }
  void disableAll() {
    events_ &= kNoneEvent;
    update();
  }

  // 返回fd当前的事件状态
  bool isNoneEvent() const { return events_ == kNoneEvent; }
  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }

  /**
   * for Poller
   * const int kNew = -1;     // fd还未被poller监视
   * const int kAdded = 1;    // fd正被poller监视中
   * const int kDeleted = 2;  // fd被移除poller
   */
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // one lool per thread
  EventLoop *ownerLoop() { return loop_; }

private:
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

private:
  /**
   * const int Channel::kNoneEvent = 0;
   * const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
   * const int Channel::kWriteEvent = EPOLLOUT;
   */
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop *loop_; // 该Channel属于的EventLoop
  const int fd_;    // 该Channel负责该文件描述符的IO事件分发
  int events_;      // 关心的IO事件,由用户设置 bit pattern
  int revents_; // 目前活动的事件,由EventLoop/Poller设置 bit pattern
  int index_;   // used by Poller. fd_在Poller::pollfds_里的下标

  std::weak_ptr<void>
      tie_; // 弱指针指向TcpConnection(必要时升级为shared_ptr多一份引用计数，避免用户误删)
  bool tied_; // 标志此 Channel 是否被调用过 Channel::tie 方法

  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_CHANNEL_H