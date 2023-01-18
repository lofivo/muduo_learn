#include "src/net/Connector.h"
#include "src/net/EventLoop.h"

#include <stdio.h>
#include <unistd.h>

mymuduo::EventLoop* g_loop;

void connectCallback(int sockfd)
{
  printf("connected.\n");
  g_loop->quit();
}

int main(int argc, char* argv[])
{
  mymuduo::EventLoop loop;
  g_loop = &loop;
  mymuduo::InetAddress addr("127.0.0.1", 9981);
  std::shared_ptr<mymuduo::Connector> connector(new mymuduo::Connector(&loop, addr));
  connector->setNewConnectionCallback(connectCallback);
  connector->start();

  loop.loop();
}
