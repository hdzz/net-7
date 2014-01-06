#ifndef NET_BASE_BUFFER_H_
#define NET_BASE_BUFFER_H_

#include <WinSock2.h>

namespace net {

const int kAsyncTypeTcpAccept = 1;
const int kAsyncTypeTcpSend = 2;
const int kAsyncTypeTcpRecv = 3;
const int kAsyncTypeUdpSend = 4;
const int kAsyncTypeUdpRecv = 5;

class BaseBuffer {
 public:
  BaseBuffer() { ResetBuffer(); }
  void ResetBuffer(){
    memset(&ovlp_, 0, sizeof(ovlp_));
    handle_ = 0;
    async_type_ = 0;
    buffer_size_ = 0;
  }
  LPOVERLAPPED ovlp() { return &ovlp_; }
  unsigned long handle() { return handle_; }
  void set_handle(unsigned long value) { handle_ = value; }
  int async_type() { return async_type_; }
  void set_async_type(int value) { async_type_ = value; }
  int buffer_size() { return buffer_size_; }
  void set_buffer_size(int value) { buffer_size_ = value; }

 private:
  OVERLAPPED ovlp_;
  unsigned long handle_;
  int async_type_;
  int buffer_size_;
};

} // namespace net

#endif	// NET_BASE_BUFFER_H_