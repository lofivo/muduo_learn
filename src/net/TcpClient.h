#ifndef MYMUDUO_NET_TCPCLIENT_H
#define MYMUDUO_NET_TCPCLIENT_H
#include "src/net/TcpConnection.h"
#include <mutex>
namespace mymuduo {
class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;
class TcpClient : noncopyable {
public:
  TcpClient(EventLoop *loop, const InetAddress &serverAddr,
            const std::string &nameArg = std::string());
  ~TcpClient();

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_;
  }

  EventLoop *getLoop() const { return loop_; }
  bool retry() const { return retry_; }
  void enableRetry() { retry_.store(true); }

  const std::string &name() const { return name_; }

  void setConnectionCallback(ConnectionCallback cb) {
    connectionCallback_ = std::move(cb);
  }
  void setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
  }
  void setWriteCompleteCallback(WriteCompleteCallback cb) {
    writeCompleteCallback_ = std::move(cb);
  }

private:
  void newConnection(int sockfd);

  void removeConnection(const TcpConnectionPtr &conn);

private:
  EventLoop *loop_;
  ConnectorPtr connector_;
  const std::string name_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  std::atomic_bool retry_;
  std::atomic_bool connect_;
  int nextConnId_;
  TcpConnectionPtr connection_;
  mutable std::mutex mutex_;
};
} // namespace mymuduo
#endif // MYMUDUO_NET_TCPCLIENT_H
