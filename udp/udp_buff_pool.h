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
  // ��ʼ�����������
  void Init();
  void Clear();

  // �ӻ������ȡ����Ӧ����
  UdpSendBuff* GetSendBuffer();
	UdpRecvBuff* GetRecvBuffer();

  // �黹��Ӧ���嵽�����
	void ReturnSendBuff(UdpSendBuff* buffer);
	void ReturnRecvBuff(UdpRecvBuff* buffer);

 private:
	UdpBuffPool(const UdpBuffPool&);	// ���ɿ��������ɸ�ֵ
	UdpBuffPool& operator=(const UdpBuffPool&);	
	void ExpandSendBuff();		// �����ڴ��
	void ExpandRecvBuff();
	void TryNarrowSendBuff();	// ������С�ڴ��
	void TryNarrowRecvBuff();

 private:
	std::list<UdpSendBuff*> send_buffer_;	// ���з��ͻ���
	std::list<UdpRecvBuff*> recv_buffer_;	// ���н��ջ���
	std::mutex send_buff_lock_;						// ���ͻ�����
	std::mutex recv_buff_lock_;						// ���ջ�����
};

} // namespace net

#endif // NET_UDP_BUFF_POOL_H_