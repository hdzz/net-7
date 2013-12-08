#ifndef NET_TCP_HEADER_H_
#define NET_TCP_HEADER_H_

#include <WinSock2.h>

namespace net {

const unsigned long kPacketFlag = 0xfdfdfdfd;

// TCP包头
class TcpHeader {
 public:
	TcpHeader() {
		packet_flag_ = kPacketFlag;
		packet_size_ = 0;
		checksum_ = 0;
	}
	void Init(unsigned long packet_size) {	// 从内存数据初始化包头
		set_packet_size(packet_size);
		::htonl(packet_flag_);
		::htonl(packet_size_);
		::htonl(checksum_);
	}
	bool Init(const char* data, int size) {	// 从网络数据初始化包头
		if (data == nullptr || size != sizeof(*this)) {
			return false;
		}
		memcpy(this, data, size);
		::ntohl(packet_flag_);
		::ntohl(packet_size_);
		::ntohl(checksum_);
		if (packet_flag_ != kPacketFlag) {
			return false;
		}
		return true;
	}
	operator char* () { return (char*)(this); }
	unsigned long packet_size() { return packet_size_; }
	void set_packet_size(unsigned long value) { packet_size_ = value; }

 private:
	unsigned long packet_flag_; // 分包标志
	unsigned long packet_size_; // 包大小
	unsigned long checksum_;    // 校验
};
const int kTcpHeadSize = sizeof(TcpHeader);		// TCP包头大小

} // namespace net

#endif // NET_TCP_HEADER_H_