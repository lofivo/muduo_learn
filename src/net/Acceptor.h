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

public:
  Acceptor(EventLoop *loop, const InetAddress &ListenAddr,
           bool reuseport = true);
  ~Acceptor();
  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }

  void listen();

  bool listening() const { return listening_; }

private:
  void handleRead();

private:
  EventLoop *loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listening_; // 是否正在监听的标志
  int idleFd_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_ACCEPTOR_H
