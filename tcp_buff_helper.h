#ifndef NET_TCP_BUFF_HELPER_H_
#define NET_TCP_BUFF_HELPER_H_

#include "tcp_buffer.h"
#include <vector>

namespace net {

const unsigned long kPacketFlag = 0xfdfdfdfd;

// TCP包头
struct TcpHead {
	TcpHead() {
    packet_flag_ = kPacketFlag;
    packet_size_ = 0;
    checksum_ = 0;
  }
	unsigned long packet_flag_; // 分包标志
	unsigned long packet_size_; // 包大小
  unsigned long checksum_;    // 校验
};

// 根据上层包大小计算需要多少个发送缓冲
int CalcSendBuffNum(int packet_size);

// 填充所有发送缓冲
void FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer);

} // namespace net

#endif	// NET_TCP_BUFF_HELPER_H_