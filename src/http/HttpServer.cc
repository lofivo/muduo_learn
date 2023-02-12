#include "src/base/Timestamp.h"
#include "src/http/HttpConnection.h"
#include "src/logger/Logging.h"
#include "src/logger/AsyncLogging.h"
#include "src/net/Acceptor.h"
#include "src/net/EventLoop.h"
#include "src/net/TcpConnection.h"
#include "src/net/TcpServer.h"

using namespace mymuduo;

int main() {
  Logger::setLogLevel(Logger::TRACE);
  Logger::useAsyncLog("./log/http", 1024 * 1024);
  EventLoop loop;
  InetAddress listenAddr("0.0.0.0", 8888);
  TcpServer server(&loop, listenAddr, "HttpServer");

  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);

  server.setThreadNum(4);
  server.start();
  loop.loop();
}