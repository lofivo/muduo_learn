#include "src/net/InetAddress.h"


using namespace mymuduo;

InetAddress::InetAddress(uint16_t port) {
  ::bzero(&addr_, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_port = ::htons(port);
  addr_.sin_addr.s_addr = ::inet_addr("0.0.0.0");
}

InetAddress::InetAddress(std::string ip, uint16_t port) {
  ::bzero(&addr_, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_port = ::htons(port);
  addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
}

InetAddress::InetAddress(const sockaddr_in &addr) : addr_(addr) {}

std::string InetAddress::toIp() const {
  char buf[64]{0};
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
  return buf;
}

uint16_t InetAddress::toPort() const { return ::ntohs(addr_.sin_port); }

std::string InetAddress::toIpPort() const {
  std::string ret = toIp() + ":" + std::to_string(toPort());
  return ret;
}

sockaddr_in InetAddress::getLocalAddr(int sockfd) {
  sockaddr_in localaddr{0};
  socklen_t addrlen = sizeof localaddr;
  if (::getsockname(sockfd, (sockaddr *)&localaddr, &addrlen)) {
    LOG_SYSERR << "Socket::getLocalAddr";
  }
  return localaddr;
}

sockaddr_in InetAddress::getPeerAddr(int sockfd) {
  sockaddr_in peeraddr{0};
  socklen_t addrlen = sizeof peeraddr;
  if (::getpeername(sockfd, (sockaddr *)&peeraddr, &addrlen)) {
    LOG_SYSERR << "Socket::getLocalAddr";
  }
  return peeraddr;
}