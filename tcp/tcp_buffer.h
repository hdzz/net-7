#ifndef NET_TCP_BUFFER_H_
#define NET_TCP_BUFFER_H_

#include "base_buffer.h"
#include "tcp_header.h"
#include <functional>

namespace net {

const int kTcpAcceptBuffSize = 64;
const int kTcpBufferSize = 64 * 1024;

class TcpSendBuffer : public BaseBuffer {
 public:
  TcpSendBuffer() {
    BaseBuffer::ResetBuffer();
    set_async_type(kAsyncTypeTcpSend);
    buffer_ = nullptr;
    deleter_ = nullptr;
  }
  ~TcpSendBuffer() {
    if (buffer_ != nullptr) {
      if (deleter_) {
        deleter_(buffer_);
      }
    }
  }
  bool Init(const char* buffer, int size, std::function<void (char*)> deleter) {
    if (buffer == nullptr || size == 0 || !deleter) {
      return false;
    }
    buffer_ = const_cast<char*>(buffer);
    deleter_ = deleter;
    header_.Init(size);
    set_buffer_size(size);
    return true;
  }
  const TcpHeader* header() { return &header_; }
  const char* buffer() { return buffer_; }

 private:
  TcpHeader header_;
  char* buffer_;
  std::function<void (char*)> deleter_;
};

class TcpRecvBuffer : public BaseBuffer {
public:
  TcpRecvBuffer() {
    ResetBuffer();
  }
  void ResetBuffer() {
    BaseBuffer::ResetBuffer();
    set_async_type(kAsyncTypeTcpRecv);
    set_buffer_size(sizeof(buffer_));
  }
  char* buffer() { return buffer_; }

private:
  char buffer_[kTcpBufferSize];
};

class TcpAcceptBuffer : public BaseBuffer {
public:
  TcpAcceptBuffer() {
    ResetBuffer();
  }
  void ResetBuffer() {
    BaseBuffer::ResetBuffer();
    set_async_type(kAsyncTypeTcpAccept);
    set_buffer_size(sizeof(buffer_));
    accept_socket_ = nullptr;
  }
  char* buffer() { return buffer_; }
  void set_accept_socket(void* value) { accept_socket_ = value; }
  void* accept_socket() const { return accept_socket_; }

private:
  char buffer_[kTcpAcceptBuffSize];
  void* accept_socket_;
};

} // namespace net

#endif	// NET_TCP_BUFFER_H_