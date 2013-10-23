#ifndef NET_TCP_BUFF_POOL_H_
#define NET_TCP_BUFF_POOL_H_

#include "tcp_buffer.h"
#include <list>
#include <mutex>
#include <vector>

namespace net {

const unsigned long kPacketFlag = 0xfdfdfdfd;
const int kTCPSendBuffPoolSize = 1024;
const int kTCPRecvBuffPoolSize = 1024;
const int kAcceptBuffPoolSize = 64;

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
const int kTcpHeadSize = sizeof(TcpHead);		// TCP��ͷ��С

class TcpBuffPool {
 public:
  TcpBuffPool();
  ~TcpBuffPool();

  // ��ʼ�����������
  bool Init();
  void Clear();

  // �����ϲ����С������Ҫ���ٸ����ͻ���
  int CalcSendBuffNum(int packet_size);

  // ������з��ͻ���
  void FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer);

  // �ӻ������ȡ����Ӧ����
  AcceptBuff* GetAcceptBuffer();
  std::vector<TcpSendBuff*> GetSendBuffer(const char* buffer, int size);
  TcpRecvBuff* GetRecvBuffer();

  // �黹��Ӧ���嵽�����
  void ReturnAcceptBuff(AcceptBuff* buffer);
  void ReturnTcpSendBuff(TcpSendBuff* buffer);
  void ReturnTcpRecvBuff(TcpRecvBuff* buffer);

private:
  std::list<TcpSendBuff*> send_buffer_;	  // ���з��ͻ���
  std::list<TcpRecvBuff*> recv_buffer_;	  // ���н��ջ���
  std::list<AcceptBuff*> accept_buffer_;  // ����accept����
  std::mutex send_buff_lock_;             // ���ͻ�����
  std::mutex recv_buff_lock_;             // ���ջ�����
  std::mutex accept_buff_lock_;           // accept������
};

} // namespace net

#endif // NET_TCP_BUFF_POOL_H_