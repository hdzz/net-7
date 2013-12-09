#ifndef NET_UDP_BUFFER_H_
#define NET_UDP_BUFFER_H_

#include "base_buffer.h"

namespace net {

const int kUdpBufferSize = 8 * 1024;

class UdpSendBuff : public BaseBuffer {
 public:
	 UdpSendBuff() {
		ResetBuffer();
	}
	bool set_buffer(const char* buffer, int size) {
		if (buffer == nullptr || size == 0) {
			return false;
		}
		memcpy(buffer_, buffer, size);
		set_buffer_size(size);
		return true;
	}
	void ResetBuffer() {
		BaseBuffer::ResetBuffer();
		set_async_type(kAsyncTypeUdpSend);
	}
	char* buffer() { return buffer_; }

 private:
	char buffer_[kUdpBufferSize];
};

class UdpRecvBuff : public BaseBuffer {
 public:
	 UdpRecvBuff() {
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
	PINT addr_size() { return &addr_size_;  }

 private:
	char buffer_[kUdpBufferSize];
	SOCKADDR_IN from_addr_;
	INT addr_size_;
};

} // namespace net

#endif	// NET_UDP_BUFFER_H_