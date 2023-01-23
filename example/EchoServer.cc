#include "src/net/TcpServer.h"
using namespace mymuduo;
void onConnect(const TcpConnectionPtr &conn) {
  LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
  conn->setTcpNoDelay(true);
}
void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp t) {
  conn->send(buf);
}
int main() {
  EventLoop loop;
  InetAddress serverAddr("0.0.0.0", 8888);
  TcpServer server(&loop, serverAddr);
  server.setConnectionCallback(::onConnect);
  server.setMessageCallback(::onMessage);
  server.start();
  loop.loop();
  return 0;
}
