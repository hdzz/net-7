#ifndef NET_UDP_BUFFER_H_
#define NET_UDP_BUFFER_H_

#include "base_buffer.h"

namespace net {

const int kUdpBufferSize = 8 * 1024;

class UdpSendBuffer : public BaseBuffer {
 public:
  UdpSendBuffer() {
    BaseBuffer::ResetBuffer();
    set_async_type(kAsyncTypeUdpSend);
    buffer_ = nullptr;
    deleter_ = nullptr;
  }
  ~UdpSendBuffer() {
    if (buffer_ != nullptr) {
      if (deleter_) {
        deleter_(buffer_);
      }
    }
  }
  bool Init(const char* buffer, int size, std::function<void(char*)> deleter) {
    if (buffer == nullptr || size == 0 || !deleter) {
      return false;
    }
    buffer_ = const_cast<char*>(buffer);
    deleter_ = deleter;
    set_buffer_size(size);
    return true;
  }
  const char* buffer() { return buffer_; }

 private:
   char* buffer_;
   std::function<void(char*)> deleter_;
};

class UdpRecvBuffer : public BaseBuffer {
 public:
  UdpRecvBuffer() {
    ResetBuffer();
  }
  void ResetBuffer() {
    BaseBuffer::ResetBuffer();
    set_async_type(kAsyncTypeUdpRecv);
    set_buffer_size(sizeof(buffer_));
    memset(&from_addr_, 0, sizeof(from_addr_));
    addr_size_ = sizeof(from_addr_);
  }
  char* buffer() { return buffer_; }
  PSOCKADDR_IN from_addr() { return &from_addr_; }
  PINT addr_size() { return &addr_size_; }

 private:
  char buffer_[kUdpBufferSize];
  SOCKADDR_IN from_addr_;
  INT addr_size_;
};

} // namespace net

#endif	// NET_UDP_BUFFER_H_