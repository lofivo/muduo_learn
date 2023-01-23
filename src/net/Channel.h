#ifndef MYMUDUO_NET_CHANNEL_H
#define MYMUDUO_NET_CHANNEL_H

#include <functional>
#include <memory>
#include <sys/epoll.h>

#include "src/base/noncopyable.h"
#include "src/logger/Logging.h"
#include "src/net/Callbacks.h"

namespace mymuduo {
class EventLoop;
class Channel : noncopyable {
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;

public:
  Channel(EventLoop *loop, int fd);
  ~Channel();
  // fd得到poller通知以后，处理事件的回调函数
  void handleEvent(Timestamp receiveTime);

  // 设置回调函数对象
  // 使用右值引用，延长了cb对象的生命周期，避免了拷贝操作
  void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  void tie(const std::shared_ptr<void> &);

  int fd() const { return fd_; }                  // 返回封装的fd
  int events() const { return events_; }          // 返回感兴趣的事件
  void set_revents(int revt) { revents_ = revt; } // 设置Poller返回的发生事件

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

  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // one lool per thread
  EventLoop *ownerLoop() { return loop_; }
  void remove();

  // for debug
  std::string reventsToString() const;
  std::string eventsToString() const;

private:
  static std::string eventsToString(int fd, int ev);
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

private:
  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop *loop_; // 该Channel属于的EventLoop
  const int fd_;    // fd, Poller监听对象
  int events_;      // 注册fd感兴趣的事件
  int revents_;     // poller返回的具体发生的事件
  int index_;       // 在Poller上注册的情况
  bool addedToLoop_;
  bool eventHandling_;
  // 弱指针指向TcpConnection(必要时升级为shared_ptr多一份引用计数，避免用户误删)
  std::weak_ptr<void> tie_;
  // 标志此 Channel 是否被调用过 Channel::tie 方法
  bool tied_;

  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};
} // namespace mymuduo

#endif // MYMUDUO_NET_CHANNEL_H