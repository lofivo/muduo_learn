#include "src/net/TcpServer.h"
#include "src/net/EventLoop.h"
#include "src/net/InetAddress.h"
#include <stdio.h>
#include <unistd.h>
std::string message1;
std::string message2;

void onConnection(const mymuduo::TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("onConnection(): tid=%d new connection [%s] from %s\n",
           gettid(),
           conn->name().c_str(),
           conn->peerAddress().toIpPort().c_str());
    if (!message1.empty())
      conn->send(message1);
    if (!message2.empty())
      conn->send(message2);
    conn->shutdown();
  }
  else
  {
    printf("onConnection(): tid=%d connection [%s] is down\n",
           gettid(),
           conn->name().c_str());
  }
}

void onMessage(const mymuduo::TcpConnectionPtr& conn,
               mymuduo::Buffer* buf,
               mymuduo::Timestamp receiveTime)
{
  printf("onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
         gettid(),
         buf->readableBytes(),
         conn->name().c_str(),
         receiveTime.toFormattedString().c_str());

  buf->retrieveAll();
}

int main(int argc, char* argv[])
{
  printf("main(): pid = %d\n", getpid());

  int len1 = 100;
  int len2 = 200;

  if (argc > 2)
  {
    len1 = atoi(argv[1]);
    len2 = atoi(argv[2]);
  }

  message1.resize(len1);
  message2.resize(len2);
  std::fill(message1.begin(), message1.end(), 'A');
  std::fill(message2.begin(), message2.end(), 'B');

  mymuduo::InetAddress listenAddr(9981);
  mymuduo::EventLoop loop;

  mymuduo::TcpServer server(&loop, listenAddr);
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  if (argc > 3) {
    server.setThreadNum(atoi(argv[3]));
  }
  server.start();

  loop.loop();
}
