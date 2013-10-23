#ifndef NET_TCP_BUFFER_H_
#define NET_TCP_BUFFER_H_

#include "base_buffer.h"

namespace net {

const int kAcceptBuffSize = 64;
const int kTcpBufferSize = 64 * 1024;

class TcpSendBuff : public BaseBuffer {
 public:
	TcpSendBuff() {}
	bool set_buffer(int offset, const char* buffer, int size) {
    auto total_size = size + offset;
		if (buffer == nullptr || total_size > sizeof(buffer_)) {
			return false;
		}
    auto current_pos = buffer_ + offset;
		memcpy(current_pos, buffer, size);
		return true;
	}
	const char* buffer() { return buffer_; }
	void ResetBuffer() { BaseBuffer::ResetBuffer(); }

 private:
	char buffer_[kTcpBufferSize];
};

class TcpRecvBuff : public BaseBuffer {
 public:
	TcpRecvBuff() { buffer_size_ = sizeof(buffer_); }
	const char* buffer() { return buffer_; }
	void ResetBuffer() {
		BaseBuffer::ResetBuffer();
		buffer_size_ = sizeof(buffer_);
	}

 private:
	char buffer_[kTcpBufferSize];
};

class AcceptBuff : public BaseBuffer {
 public:
	AcceptBuff() { buffer_size_ = sizeof(buffer_); }
	const char* buffer() { return buffer_; }
	unsigned long accept_link() { return accept_link_; }
	void set_accept_link(unsigned long value) { accept_link_ = value; }
	void* accept_socket() { return accept_socket_; }
	void set_accept_socket(void* value) { accept_socket_ = value; }
	void ResetBuffer() {
		BaseBuffer::ResetBuffer();
		buffer_size_ = sizeof(buffer_);
		accept_link_ = ~0;
		accept_socket_ = nullptr;
	}

 private:
	char buffer_[kAcceptBuffSize];
	unsigned long accept_link_;
	void* accept_socket_;
};

} // namespace net

#endif	// NET_TCP_BUFFER_H_