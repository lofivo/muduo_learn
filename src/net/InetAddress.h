#ifndef MYMUDUO_NET_INETADDRESS_H
#define MYMUDUO_NET_INETADDRESS_H

#include "src/logger/Logging.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <string>

namespace mymuduo {
class InetAddress {
public:
  explicit InetAddress(const std::string ip = "127.0.0.1", uint16_t port = 0);

  explicit InetAddress(const struct sockaddr_in &addr);

  // "xxx.xxx.xxx.xxx"
  std::string toIp() const;

  uint16_t toPort() const;

  // "xxx.xxx.xxx.xxx:xxx"
  std::string toIpPort() const;

  const sockaddr_in *getSockAddr() const { return &addr_; }

  void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

public:
  static sockaddr_in getLocalAddr(int sockfd);
  static sockaddr_in getPeerAddr(int sockfd);

private:
  struct sockaddr_in addr_;
};

} // namespace mymuduo

#endif // MYMUDUO_NET_INETADDRESS_H
