#ifndef MYMUDUO_NET_CONNECTOR_H
#define MYMUDUO_NET_CONNECTOR_H

#include <atomic>
#include <functional>
#include <memory>

#include "src/base/noncopyable.h"
#include "src/net/InetAddress.h"

namespace mymuduo {
class Channel;
class EventLoop;
class Connector : noncopyable, public std::enable_shared_from_this<Connector> {
public:
  using NewConnectionCallback = std::function<void(int sockfd)>;

public:
  Connector(EventLoop *loop, const InetAddress &serverAddr);
  ~Connector();
  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }
  void start();   // can be called in any thread
  void restart(); // must be called in loop thread
  void stop();    // can be called in any thread

  bool connected() const {
    return connect_.load() && state_.load() != kDisconnected;
  }

  const InetAddress &serverAddress() const { return serverAddr_; }

private:
  enum States { kDisconnected, kConnecting, kConnected };
  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

private:
  inline static const int kMaxRetryDelayMs = 30 * 1000;
  inline static const int kInitRetryDelayMs = 500;
  EventLoop *loop_;
  InetAddress serverAddr_;
  std::atomic_bool connect_;
  std::atomic_int state_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  int retryDelayMs_;
};
} // namespace mymuduo

#endif // MYMUDUO_NET_CONNECTOR_H