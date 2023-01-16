#ifndef MYMUDUO_NET_TCPCONNECTION_H
#define MYMUDUO_NET_TCPCONNECTION_H

#include "src/base/noncopyable.h"
#include "src/net/Callbacks.h"
#include "src/net/InetAddress.h"

#include <atomic>
#include <memory>

namespace mymuduo {
class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &localAddress() const { return localAddr_; }
  const InetAddress &peerAddress() const { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

  /// Internal use only.

  // called when TcpServer accepts a new connection
  void connectEstablished(); // should be called only once

private:
  enum StateE {
    kConnecting,
    kConnected,
  };

  void setState(StateE s) { state_ = s; }
  void handleRead();

  EventLoop *loop_;
  std::string name_;
  std::atomic_int state_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  InetAddress localAddr_;
  InetAddress peerAddr_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
} // namespace mymuduo

#endif // MYMUDUO_NET_TCPCONNECTION_H
