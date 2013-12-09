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
  // 初始化、清理缓冲池
  void Init();
  void Clear();

  // 从缓冲池中取出相应缓冲
  UdpSendBuff* GetSendBuffer();
	UdpRecvBuff* GetRecvBuffer();

  // 归还相应缓冲到缓冲池
	void ReturnSendBuff(UdpSendBuff* buffer);
	void ReturnRecvBuff(UdpRecvBuff* buffer);

 private:
	UdpBuffPool(const UdpBuffPool&);	// 不可拷贝，不可赋值
	UdpBuffPool& operator=(const UdpBuffPool&);	
	void ExpandSendBuff();		// 扩大内存池
	void ExpandRecvBuff();
	void TryNarrowSendBuff();	// 尝试缩小内存池
	void TryNarrowRecvBuff();

 private:
	std::list<UdpSendBuff*> send_buffer_;	// 所有发送缓冲
	std::list<UdpRecvBuff*> recv_buffer_;	// 所有接收缓冲
	std::mutex send_buff_lock_;						// 发送缓冲锁
	std::mutex recv_buff_lock_;						// 接收缓冲锁
};

} // namespace net

#endif // NET_UDP_BUFF_POOL_H_