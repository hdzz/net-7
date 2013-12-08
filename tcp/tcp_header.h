#ifndef NET_TCP_HEADER_H_
#define NET_TCP_HEADER_H_

#include <WinSock2.h>

namespace net {

const unsigned long kPacketFlag = 0xfdfdfdfd;

// TCP��ͷ
class TcpHeader {
 public:
	TcpHeader() {
		packet_flag_ = kPacketFlag;
		packet_size_ = 0;
		checksum_ = 0;
	}
	void Init(unsigned long packet_size) {	// ���ڴ����ݳ�ʼ����ͷ
		set_packet_size(packet_size);
		::htonl(packet_flag_);
		::htonl(packet_size_);
		::htonl(checksum_);
	}
	bool Init(const char* data, int size) {	// ���������ݳ�ʼ����ͷ
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
	unsigned long packet_flag_; // �ְ���־
	unsigned long packet_size_; // ����С
	unsigned long checksum_;    // У��
};
const int kTcpHeadSize = sizeof(TcpHeader);		// TCP��ͷ��С

} // namespace net

#endif // NET_TCP_HEADER_H_