#ifndef MYMUDUO_HTTP_HTTPCONNECTION_H
#define MYMUDUO_HTTP_HTTPCONNECTION_H
#include "src/base/noncopyable.h"
#include "src/net/Callbacks.h"
#include "src/net/TimerQueue.h"
#include "src/net/TimerId.h"

#include <map>
#include <memory>
#include <string>
#include <sys/stat.h>

namespace mymuduo {

class HttpConnection : noncopyable {
public:
  enum ParseState {
    kRequestLine,
    kHeader,
    kBody,
    kFinish,
  };

  enum HttpCode {
    kNoRequest,
    kGetRequest,
    kBadRequest, // 格式错误
    kForbidden,
    kNoResource,
  };

  static const std::map<int, std::string> kResponses;
  static const std::map<std::string, std::string> kMimeType;
  // static const std::map<std::string, bool> kPostUserVerify;

  HttpConnection(const std::string &sourceDir);
  ~HttpConnection() = default;

  void processMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp);
  void setTimerId(TimerId timerId) { timerId_ = timerId; }
  TimerId getTimerId() const { return timerId_; }

private:
  HttpCode parseRequest(Buffer *inputBuf);
  HttpCode parseRequestLine(const std::string &line);
  HttpCode parseRequestHeader(const std::string &line);
  HttpCode parseRequestBody(const std::string &line);

  HttpCode parsePost();
  HttpCode parseFromUrlEncode();
  HttpCode userVerify();

  void makeResponse(Buffer *outputBuf, HttpCode parseRet);
  void initResponse(HttpCode httpCode);
  void makeResponseLine(Buffer *outputBuf);
  void makeResponseHeader(Buffer *outputBuf);
  void makeResponseBody(Buffer *outputBuf);

  void resetState();

  ParseState parseState_;

  std::string method_, path_, version_; // request line
  std::map<std::string, std::string> header_;
  std::string body_;
  std::map<std::string, std::string> post_;

  int responseCode_;
  bool keepAlive_;
  struct stat requestFileStat_;

  TimerId timerId_; // for the shutdown in timeout

  const std::string kSourceDir;
};

typedef std::shared_ptr<HttpConnection> HttpConnectionPtr;

void onConnection(const TcpConnectionPtr &conn);
void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp t);
} // namespace mymuduo

#endif // MYMUDUO_HTTP_HTTPCONNECTION_H