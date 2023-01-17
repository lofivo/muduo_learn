#ifndef MYMUDUO_NET_TCPCONNECTION_H
#define MYMUDUO_NET_TCPCONNECTION_H

#include "src/base/Timestamp.h"
#include "src/base/noncopyable.h"
#include "src/logger/Logging.h"
#include "src/net/Buffer.h"
#include "src/net/Callbacks.h"
#include "src/net/InetAddress.h"

#include <any>
#include <atomic>
#include <cstring>
#include <memory>
#include <string>

namespace mymuduo {
class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &localAddress() const { return localAddr_; }
  const InetAddress &peerAddress() const { return peerAddr_; }

  bool connected() const { return state_ == kConnected; }

  void send(const std::string &msg);
  void send(const void *msg, size_t len);
  void send(Buffer *buf);

  void shutdown();

  void forceClose();

  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallback_ = cb;
  }

  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }

  void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) {
    highWaterMarkCallback_ = cb;
  }

  void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

  Buffer *inputBuffer() { return &inputBuffer_; }

  Buffer *outputBuffer() { return &outputBuffer_; }

  // TcpServer会调用
  void connectEstablished(); // 连接建立
  void connectDestroyed();   // 连接销毁

private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void setState(StateE state) { state_.store(state); }
  const char* stateToString() const;
  void sendInLoop(const void *data, size_t len);
  void shutdownInLoop();
  void forceCloseInLoop();

  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();

private:
  EventLoop *loop_;
  const std::string name_;
  std::atomic_int state_;
  bool reading_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;

  const InetAddress localAddr_; // 本服务器地址
  const InetAddress peerAddr_;  // 对端地址

  /**
   * 用户自定义的这些事件的处理函数，然后传递给 TcpServer
   * TcpServer 再在创建 TcpConnection 对象时候设置这些回调函数到 TcpConnection中
   */
  ConnectionCallback connectionCallback_;       // 有新连接时的回调
  MessageCallback messageCallback_;             // 有读写消息时的回调
  WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
  CloseCallback closeCallback_; // 客户端关闭连接的回调

  HighWaterMarkCallback highWaterMarkCallback_; // 超出水位实现的回调
  size_t highWaterMark_;

  Buffer inputBuffer_;  // 读取数据的缓冲区
  Buffer outputBuffer_; // 发送数据的缓冲区
};
} // namespace mymuduo
#endif // MYMUDUO_NET_TCPCONNECTION_H
