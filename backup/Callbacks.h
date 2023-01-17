#ifndef MYMUDUO_NET_CALLBACKS_H
#define MYMUDUO_NET_CALLBACKS_H

#include "src/base/Timestamp.h"

#include <functional>
#include <memory>

namespace mymuduo {
class Buffer;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback =
    std::function<void(const TcpConnectionPtr &, size_t)>;

using MessageCallback =
    std::function<void(const TcpConnectionPtr &, const char *, size_t)>;

} // namespace mymuduo

#endif // MYMUDUO_NET_CALLBACKS_H