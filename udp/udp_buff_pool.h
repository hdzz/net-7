#ifndef NET_UDP_BUFF_POOL_H_
#define NET_UDP_BUFF_POOL_H_

#include "udp_buffer.h"
#include <list>
#include <mutex>
#include <vector>

namespace net {

class UdpBuffPool {
 public:
  UdpBuffPool();
  ~UdpBuffPool();
  void Init();
  void Clear();
  UdpSendBuff* GetSendBuffer();
  UdpRecvBuff* GetRecvBuffer();
  void ReturnSendBuff(UdpSendBuff* buffer);
  void ReturnRecvBuff(UdpRecvBuff* buffer);

 private:
  UdpBuffPool(const UdpBuffPool&);
  UdpBuffPool& operator=(const UdpBuffPool&);
  void ExpandSendBuff();
  void ExpandRecvBuff();
  void TryNarrowSendBuff();
  void TryNarrowRecvBuff();

 private:
  std::list<UdpSendBuff*> send_buffer_;
  std::list<UdpRecvBuff*> recv_buffer_;
  std::mutex send_buff_lock_;
  std::mutex recv_buff_lock_;
  int send_buff_size_;
  int recv_buff_size_;
};

} // namespace net

#endif // NET_UDP_BUFF_POOL_H_