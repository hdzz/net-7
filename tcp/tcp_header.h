#ifndef NET_TCP_HEADER_H_
#define NET_TCP_HEADER_H_

#include <WinSock2.h>

namespace net {

const unsigned long kTcpPacketFlag = 0xfdfdfdfd;
const unsigned long kMaxTcpSendPacketSize = 16 * 1024 * 1024;

class TcpHeader {
 public:
  TcpHeader() {
    packet_flag_ = kTcpPacketFlag;
    packet_size_ = 0;
    checksum_ = 0;
  }
  void Init(unsigned long packet_size) {
    packet_size_ = packet_size;
    packet_flag_ = ::htonl(packet_flag_);
    packet_size_ = ::htonl(packet_size_);
    checksum_ = ::htonl(checksum_);
  }
  bool Init(const char* data, int size) {
    if (data == nullptr || size != sizeof(*this)) {
      return false;
    }
    memcpy(this, data, size);
    packet_flag_ = ::ntohl(packet_flag_);
    packet_size_ = ::ntohl(packet_size_);
    checksum_ = ::ntohl(checksum_);
    if (packet_flag_ != kTcpPacketFlag || packet_size_ > kMaxTcpSendPacketSize) {
      return false;
    }
    return true;
  }
  unsigned long packet_size() { return packet_size_; }

 private:
  unsigned long packet_flag_;
  unsigned long packet_size_;
  unsigned long checksum_;
};
const int kTcpHeaderSize = sizeof(TcpHeader);

} // namespace net

#endif // NET_TCP_HEADER_H_