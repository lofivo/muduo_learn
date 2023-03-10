#ifndef MYMUDUO_NET_CALLBACKS_H
#define MYMUDUO_NET_CALLBACKS_H

#include <functional>
#include <memory>

namespace mymuduo {
class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback =
    std::function<void(const TcpConnectionPtr &, size_t)>;

using MessageCallback =
    std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;

} // namespace mymuduo

#endif // MYMUDUO_NET_CALLBACKS_H