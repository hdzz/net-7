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
  void Init();
  void Clear();
  TcpAcceptBuff* GetAcceptBuffer();
  std::vector<TcpSendBuff*> GetSendBuffer(const char* buffer, int size);
  TcpRecvBuff* GetRecvBuffer();
  void ReturnAcceptBuff(TcpAcceptBuff* buffer);
  void ReturnSendBuff(TcpSendBuff* buffer);
  void ReturnRecvBuff(TcpRecvBuff* buffer);

 private:
	TcpBuffPool(const TcpBuffPool&);
	TcpBuffPool& operator=(const TcpBuffPool&);
	void ExpandAcceptBuff();
	void ExpandSendBuff();
	void ExpandRecvBuff();
	void TryNarrowAcceptBuff();
	void TryNarrowSendBuff();
	void TryNarrowRecvBuff();
	int CalcSendBuffNum(int packet_size);
	void FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer);

 private:
	std::list<TcpAcceptBuff*> accept_buffer_;
  std::list<TcpSendBuff*> send_buffer_;
  std::list<TcpRecvBuff*> recv_buffer_;
	std::mutex accept_buff_lock_;
	std::mutex send_buff_lock_;
	std::mutex recv_buff_lock_;
	int accept_buff_size_;
	int send_buff_size_;
	int recv_buff_size_;
};

} // namespace net

#endif // NET_TCP_BUFF_POOL_H_