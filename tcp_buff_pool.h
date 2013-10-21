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

  bool Init();                    // ��ʼ�������
  void Clear();                   // �������
  AcceptBuff* GetAcceptBuffer();  // �ӻ������ȡ��һ������
  TcpSendBuff* GetSendBuffer();
  TcpRecvBuff* GetRecvBuffer();
  void ReturnAcceptBuff(AcceptBuff* buffer);  // �黹һ�����嵽�����
  void ReturnTcpSendBuff(TcpSendBuff* buffer);
  void ReturnTcpRecvBuff(TcpRecvBuff* buffer);

private:
  std::list<TcpSendBuff*> send_buff_;	  // ���з��ͻ���
  std::list<TcpRecvBuff*> recv_buff_;	  // ���н��ջ���
  std::list<AcceptBuff*> accept_buff_;  // ����accept����
  std::mutex send_buff_lock_;           // ���ͻ�����
  std::mutex recv_buff_lock_;           // ���ջ�����
  std::mutex accept_buff_lock_;         // accept������
};

} // namespace net

#endif // NET_TCP_BUFF_POOL_H_