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
const int kTcpHeadSize = sizeof(TcpHead);		// TCP包头大小

class TcpBuffPool {
 public:
  TcpBuffPool();
  ~TcpBuffPool();

  // 初始化、清理缓冲池
  bool Init();
  void Clear();

  // 根据上层包大小计算需要多少个发送缓冲
  int CalcSendBuffNum(int packet_size);

  // 填充所有发送缓冲
  void FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer);

  // 从缓冲池中取出相应缓冲
  AcceptBuff* GetAcceptBuffer();
  std::vector<TcpSendBuff*> GetSendBuffer(const char* buffer, int size);
  TcpRecvBuff* GetRecvBuffer();

  // 归还相应缓冲到缓冲池
  void ReturnAcceptBuff(AcceptBuff* buffer);
  void ReturnTcpSendBuff(TcpSendBuff* buffer);
  void ReturnTcpRecvBuff(TcpRecvBuff* buffer);

private:
  std::list<TcpSendBuff*> send_buffer_;	  // 所有发送缓冲
  std::list<TcpRecvBuff*> recv_buffer_;	  // 所有接收缓冲
  std::list<AcceptBuff*> accept_buffer_;  // 所有accept缓冲
  std::mutex send_buff_lock_;             // 发送缓冲锁
  std::mutex recv_buff_lock_;             // 接收缓冲锁
  std::mutex accept_buff_lock_;           // accept缓冲锁
};

} // namespace net

#endif // NET_TCP_BUFF_POOL_H_