#ifndef NET_RES_MANAGER_H_
#define NET_RES_MANAGER_H_

#include "net.h"
#include "tcp_buff_pool.h"
#include "tcp_socket.h"
#include <map>
#include <thread>

namespace net {

class ResManager {
 public:
	ResManager();
	~ResManager();
	// ���º������ǽӿڵ�ʵ�֣����幤����ʽ��ʵ��ע��
	bool StartupNet(StartupType type);
	bool CleanupNet();
	bool CreateTcpLink(NetInterface* callback, const char* ip, int port, TcpLink& new_link);
	bool DestroyTcpLink(TcpLink link);
	bool Listen(TcpLink link, int backlog);
	bool Connect(TcpLink link, const char* ip, int port);
	bool SendTcpPacket(TcpLink link, const char* packet, int size);
	bool GetTcpLinkAddr(TcpLink link, EitherPeer type, char ip[16], int& port);
	bool SetTcpLinkAttr(TcpLink link, AttributeType type, int value);
	bool GetTcpLinkAttr(TcpLink link, AttributeType type, int& value);
	bool CreateUdpLink(NetInterface* callback, const char* ip, int port, bool broadcast, UdpLink& new_link);
	bool DestroyUdpLink(UdpLink link);
	bool SendUdpPacket(UdpLink link, const char* packet, int size, const char* ip, int port);

 private:
	ResManager(const ResManager&);	// ���������ɿ��������ɸ�ֵ
	ResManager& operator=(const ResManager&);
  
	int GetProcessorNum();                  // ��ȡ����������
	bool BindToIOCP(TcpSocket* socket);     // �󶨵�IO��ɶ˿�
  bool IocpWorker();                      // IOCP�����߳�
	TcpLink AddTcpLink(TcpSocket* socket);  // ����һ��TCP����
	bool RemoveTcpLink(TcpLink link);       // �Ƴ�һ��TCP����
	TcpSocket* FindTcpSocket(TcpLink link); // ����һ��TCP����

  // �첽Ͷ��IO
	bool AsyncMoreAccept(TcpLink link, TcpSocket* socket, int count);
	bool AsyncAccept(TcpLink link, TcpSocket* socket, AcceptBuff* buffer);
	bool AsyncTcpRecv(TcpLink link, TcpSocket* socket, TcpRecvBuff* buffer);
	
	// ���첽IO���ʱ�����ݲ�ͬ���ͷַ�����ͬ�ĺ�����������ע�ͼ�ʵ��
	bool TransferAsyncType(BaseBuffer* buffer, int transfer_size);
	bool OnTcpLinkAccept(AcceptBuff* buffer, int accept_size);
	bool OnTcpLinkSend(TcpSendBuff* buffer, int send_size);
	bool OnTcpLinkRecv(TcpRecvBuff* buffer, int recv_size);
	bool OnUdpLinkSend(BaseBuffer* buffer, int send_size);
	bool OnUdpLinkRecv(BaseBuffer* buffer, int recv_size);

  unsigned int GetPacketSize(TcpSocket* recv_sock);
  bool ParseHeader(TcpSocket* recv_sock, const char* data, int size, int& dealed_size);
  bool ParsePacket(TcpSocket* recv_sock, const char* data, int size, int& dealed_size);

 private:
	bool is_net_started_;								      // �����Ƿ��Ѿ���ʼ��
	HANDLE iocp_;												      // IOCP�ں˶�����
	std::vector<std::thread*> iocp_thread_;   // �����߳̾��
  unsigned long tcp_link_count_;			      // TCP��ڼ��������ڶ��������ҵ�keyֵ
	std::map<TcpLink, TcpSocket*> tcp_link_;	// ����TCP��ڶ���
	std::mutex tcp_link_lock_;                // ����TCP��ڶ������
  TcpBuffPool tcp_buff_pool_;               // TCP�����
};

} // namespace net

#endif	// NET_RES_MANAGER_H_