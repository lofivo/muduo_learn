#include "src/base/Thread.h"
#include "src/net/EventLoop.h"

mymuduo::EventLoop *g_loop;

void threadFunc() { g_loop->loop(); }

int main() {
  mymuduo::EventLoop loop;
  g_loop = &loop;
  mymuduo::Thread t(threadFunc);
  t.start();
  t.join();
}