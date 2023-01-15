#ifndef MYMUDUO_NET_ACCEPTOR_H
#define MYMUDUO_NET_ACCEPTOR_H

#include "src/base/noncopyable.h"
#include "src/net/Channel.h"
#include "src/net/Socket.h"

namespace mymuduo {
class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
public:
  using NewConnectionCallback =
      std::function<void(int sockfd, const InetAddress &)>;

  Acceptor(EventLoop *loop, const InetAddress &ListenAddr);

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }

  bool listenning() const { return listenning_; }

  void listen();

private:
  void handleRead();

private:
  EventLoop *loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listenning_; // 是否正在监听的标志
};

} // namespace mymuduo

#endif // MYMUDUO_NET_ACCEPTOR_H
