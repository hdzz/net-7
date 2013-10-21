#ifndef NET_BASE_BUFFER_H_
#define NET_BASE_BUFFER_H_

#include <WinSock2.h>

namespace net {

const int kAsyncTypeAccept = 1;		// 定义异步投递的类型
const int kAsyncTypeTcpSend = 2;
const int kAsyncTypeTcpRecv = 3;
const int kAsyncTypeUdpSend = 4;
const int kAsyncTypeUdpRecv = 5;

class BaseBuffer {
 public:
	BaseBuffer() { ResetBuffer(); }
	unsigned long link() { return link_; }
	void set_link(unsigned long value) { link_ = value; }
	SOCKET socket() { return socket_; }
	void set_socket(SOCKET value) { socket_ = value; }
	int async_type() { return async_type_; }
	void set_async_type(int value) { async_type_ = value; }
	int buffer_size() { return buffer_size_; }
	void set_buffer_size(int value) { buffer_size_ = value; }
	void ResetBuffer(){
		memset(&ovlp_, 0, sizeof(ovlp_));
		link_ = ~0;
		socket_ = INVALID_SOCKET;
		async_type_ = 0;
		buffer_size_ = 0;
	}

 protected:
	OVERLAPPED ovlp_;
	SOCKET socket_;
  unsigned long link_;
	int async_type_;
	int buffer_size_;
};

} // namespace net

#endif	// NET_BASE_BUFFER_H_