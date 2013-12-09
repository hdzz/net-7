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
  // ��ʼ�����������
  void Init();
  void Clear();

  // �ӻ������ȡ����Ӧ����
  AcceptBuff* GetAcceptBuffer();
  std::vector<TcpSendBuff*> GetSendBuffer(const char* buffer, int size);
  TcpRecvBuff* GetRecvBuffer();

  // �黹��Ӧ���嵽�����
  void ReturnAcceptBuff(AcceptBuff* buffer);
  void ReturnSendBuff(TcpSendBuff* buffer);
  void ReturnRecvBuff(TcpRecvBuff* buffer);

 private:
	TcpBuffPool(const TcpBuffPool&);	// ���ɿ��������ɸ�ֵ
	TcpBuffPool& operator=(const TcpBuffPool&);
	void ExpandAcceptBuff();		// �����ڴ��
	void ExpandSendBuff();
	void ExpandRecvBuff();
	void TryNarrowAcceptBuff();	// ������С�ڴ��
	void TryNarrowSendBuff();
	void TryNarrowRecvBuff();
	int CalcSendBuffNum(int packet_size);	// �����ϲ����С������Ҫ���ٸ����ͻ���
	void FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer);	// ������з��ͻ���

 private:
	std::list<AcceptBuff*> accept_buffer_;  // ����accept����
  std::list<TcpSendBuff*> send_buffer_;	  // ���з��ͻ���
  std::list<TcpRecvBuff*> recv_buffer_;		// ���н��ջ���
	std::mutex accept_buff_lock_;						// accept������
	std::mutex send_buff_lock_;							// ���ͻ�����
	std::mutex recv_buff_lock_;							// ���ջ�����
};

} // namespace net

#endif // NET_TCP_BUFF_POOL_H_