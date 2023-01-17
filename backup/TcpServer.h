#ifndef MYMUDUO_NET_TCPSERVER_H
#define MYMUDUO_NET_TCPSERVER_H

#include "src/net/Callbacks.h"
#include "src/base/noncopyable.h"
#include "src/net/TcpConnection.h"

#include <atomic>
#include <memory>
#include <unordered_map>
#include <string>

namespace mymuduo {

class Acceptor;
class EventLoop;

class TcpServer : noncopyable {
public:
  TcpServer(EventLoop *loop, const InetAddress &listenAddr);
  TcpServer(EventLoop *loop, const InetAddress &listenAddr,
            const std::string &name);
  ~TcpServer(); // force out-line dtor, for scoped_ptr members.

  /// Starts the server if it's not listenning.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress &peerAddr);

  using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

  EventLoop *loop_; // the acceptor loop
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  std::atomic_int started_;
  int nextConnId_; // always in loop thread
  ConnectionMap connections_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_TCPSERVER_H