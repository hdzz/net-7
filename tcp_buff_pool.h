#ifndef NET_TCP_BUFF_POOL_H_
#define NET_TCP_BUFF_POOL_H_

#include "tcp_buff_helper.h"
#include <list>
#include <mutex>
#include <vector>

namespace net {

const int kTCPSendBuffPoolSize = 1024;
const int kTCPRecvBuffPoolSize = 1024;
const int kAcceptBuffPoolSize = 64;

class TcpBuffPool {
 public:
  TcpBuffPool();
  ~TcpBuffPool();

  bool Init();                    // 初始化缓冲池
  void Clear();                   // 清理缓冲池
  AcceptBuff* GetAcceptBuffer();  // 从缓冲池中取出一个缓冲
  TcpSendBuff* GetSendBuffer();
  TcpRecvBuff* GetRecvBuffer();
  void ReturnAcceptBuff(AcceptBuff* buffer);  // 归还一个缓冲到缓冲池
  void ReturnTcpSendBuff(TcpSendBuff* buffer);
  void ReturnTcpRecvBuff(TcpRecvBuff* buffer);

private:
  std::list<TcpSendBuff*> send_buff_;	  // 所有发送缓冲
  std::list<TcpRecvBuff*> recv_buff_;	  // 所有接收缓冲
  std::list<AcceptBuff*> accept_buff_;  // 所有accept缓冲
  std::mutex send_buff_lock_;           // 发送缓冲锁
  std::mutex recv_buff_lock_;           // 接收缓冲锁
  std::mutex accept_buff_lock_;         // accept缓冲锁
};

} // namespace net

#endif // NET_TCP_BUFF_POOL_H_