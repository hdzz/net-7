#ifndef NET_TCP_BUFF_HELPER_H_
#define NET_TCP_BUFF_HELPER_H_

#include "tcp_buffer.h"
#include <vector>

namespace net {

const unsigned long kPacketFlag = 0xfdfdfdfd;

// TCP��ͷ
struct TcpHead {
	TcpHead() {
    packet_flag_ = kPacketFlag;
    packet_size_ = 0;
    checksum_ = 0;
  }
	unsigned long packet_flag_; // �ְ���־
	unsigned long packet_size_; // ����С
  unsigned long checksum_;    // У��
};

// �����ϲ����С������Ҫ���ٸ����ͻ���
int CalcSendBuffNum(int packet_size);

// ������з��ͻ���
void FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer);

} // namespace net

#endif	// NET_TCP_BUFF_HELPER_H_