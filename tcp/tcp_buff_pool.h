#ifndef NET_TCP_BUFF_POOL_H_
#define NET_TCP_BUFF_POOL_H_

#include "tcp_buffer.h"
#include <list>
#include <mutex>
#include <vector>

namespace net {

class TcpBuffPool {
 public:
	TcpBuffPool();
	~TcpBuffPool();
  // 初始化、清理缓冲池
  void Init();
  void Clear();

  // 从缓冲池中取出相应缓冲
  AcceptBuff* GetAcceptBuffer();
  std::vector<TcpSendBuff*> GetSendBuffer(const char* buffer, int size);
  TcpRecvBuff* GetRecvBuffer();

  // 归还相应缓冲到缓冲池
  void ReturnAcceptBuff(AcceptBuff* buffer);
  void ReturnSendBuff(TcpSendBuff* buffer);
  void ReturnRecvBuff(TcpRecvBuff* buffer);

 private:
	TcpBuffPool(const TcpBuffPool&);	// 不可拷贝，不可赋值
	TcpBuffPool& operator=(const TcpBuffPool&);
	void ExpandAcceptBuff();		// 扩大内存池
	void ExpandSendBuff();
	void ExpandRecvBuff();
	void TryNarrowAcceptBuff();	// 尝试缩小内存池
	void TryNarrowSendBuff();
	void TryNarrowRecvBuff();
	int CalcSendBuffNum(int packet_size);	// 根据上层包大小计算需要多少个发送缓冲
	void FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer);	// 填充所有发送缓冲

 private:
	std::list<AcceptBuff*> accept_buffer_;  // 所有accept缓冲
  std::list<TcpSendBuff*> send_buffer_;	  // 所有发送缓冲
  std::list<TcpRecvBuff*> recv_buffer_;		// 所有接收缓冲
	std::mutex accept_buff_lock_;						// accept缓冲锁
	std::mutex send_buff_lock_;							// 发送缓冲锁
	std::mutex recv_buff_lock_;							// 接收缓冲锁
};

} // namespace net

#endif // NET_TCP_BUFF_POOL_H_