#ifndef MYMUDUO_NET_SOCKET_H
#define MYMUDUO_NET_SOCKET_H

#include "src/base/noncopyable.h"

namespace mymuduo {

class InetAddress;

class Socket : noncopyable {
public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}

  ~Socket();

  int fd() const { return sockfd_; }

  void bindAddress(const InetAddress &localaddr);

  void listen();

  int accept(InetAddress *peeraddr);

  // 设置半关闭
  void shutdownWrite();
  // 设置全关闭
  void shutdown();
  void setTcpNoDelay(bool on); // 设置Nagel算法
  void setReuseAddr(bool on);  // 设置地址复用
  void setReusePort(bool on);  // 设置端口复用
  void setKeepAlive(bool on);  // 设置长连接

  static int createNonblockingFd();
  static int getSocketError(int sockfd);
  static bool isSelfConnect(int sockfd);

private:
  const int sockfd_;
};

} // namespace mymuduo
#endif // MYMUDUO_NET_SOCKET_H
