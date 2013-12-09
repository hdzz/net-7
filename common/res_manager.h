#ifndef NET_RES_MANAGER_H_
#define NET_RES_MANAGER_H_

#include "iocp.h"
#include "net.h"
#include "tcp_buff_pool.h"
#include "tcp_socket.h"
#include "udp_buff_pool.h"
#include "udp_socket.h"
#include <map>

namespace net {

class ResManager {
 public:
	ResManager();
	~ResManager();
	// ���º������ǽӿڵ�ʵ�֣����幤����ʽ��ʵ��ע��
	bool StartupNet();
	bool CleanupNet();
	bool CreateTcpLink(NetInterface* callback, const char* ip, int port, TcpLink& new_link);
	bool DestroyTcpLink(TcpLink link);
	bool Listen(TcpLink link, int backlog);
	bool Connect(TcpLink link, const char* ip, int port);
	bool SendTcpPacket(TcpLink link, const char* packet, int size);
	bool GetTcpLinkLocalAddr(TcpLink link, char ip[16], int& port);
	bool GetTcpLinkRemoteAddr(TcpLink link, char ip[16], int& port);
	bool CreateUdpLink(NetInterface* callback, const char* ip, int port, bool broadcast, UdpLink& new_link);
	bool DestroyUdpLink(UdpLink link);
	bool SendUdpPacket(UdpLink link, const char* packet, int size, const char* ip, int port);

 private:
	ResManager(const ResManager&);	// ���������ɿ��������ɸ�ֵ
	ResManager& operator=(const ResManager&);
  
	TcpLink AddTcpLink(TcpSocket* new_socket);	// ����һ��TCP����
	void RemoveTcpLink(TcpLink link);						// �Ƴ�һ��TCP����
	TcpSocket* FindTcpSocket(TcpLink link);			// ����һ��TCP����
	UdpLink AddUdpLink(UdpSocket* new_socket);	// ����һ��UDP����
	void RemoveUdpLink(UdpLink link);						// �Ƴ�һ��UDP����
	UdpSocket* FindUdpSocket(UdpLink link);			// ����һ��UDP����

  // �첽Ͷ��IO
	bool AsyncMoreAccept(TcpLink link, TcpSocket* socket, int count);
	bool AsyncAccept(TcpLink link, TcpSocket* socket, AcceptBuff* buffer);
	bool AsyncTcpRecv(TcpLink link, TcpSocket* socket, TcpRecvBuff* buffer);
	void OnTcpRecvNothing(TcpLink link, TcpRecvBuff* buffer, NetInterface* callback);
	void OnTcpRecvError(TcpLink link, TcpRecvBuff* buffer, NetInterface* callback);
	bool AsyncMoreUdpRecv(UdpLink link, UdpSocket* socket, int count);
	bool AsyncUdpRecv(UdpLink link, UdpSocket* socket, UdpRecvBuff* buffer);
	void OnUdpRecvError(UdpLink link, UdpRecvBuff* buffer, NetInterface* callback);
	
	// ���첽IO���ʱ�����ݲ�ͬ���ͷַ�����ͬ�ĺ�����������ע�ͼ�ʵ��
	bool TransferAsyncType(LPOVERLAPPED ovlp, DWORD transfer_size);
	bool OnTcpLinkAccept(AcceptBuff* buffer, int accept_size);
	bool OnTcpLinkSend(TcpSendBuff* buffer, int send_size);
	bool OnTcpLinkRecv(TcpRecvBuff* buffer, int recv_size);
	bool OnUdpLinkSend(UdpSendBuff* buffer, int send_size);
	bool OnUdpLinkRecv(UdpRecvBuff* buffer, int recv_size);

	unsigned int GetPacketSize(const std::vector<char>& header_stream);
  bool ParseHeader(TcpSocket* recv_sock, const char* data, int size, int& dealed_size);
  int ParsePacket(TcpSocket* recv_sock, const char* data, int size);

 private:
	bool is_net_started_;											// �����Ƿ��Ѿ���ʼ��
	IOCP iocp_;																// IOCP����
	TcpBuffPool tcp_buff_pool_;								// TCP�����
	std::mutex tcp_link_lock_;								// ����TCP��ڶ������
  unsigned long tcp_link_count_;						// TCP��ڼ��������ڶ��������ҵ�keyֵ
	std::map<TcpLink, TcpSocket*> tcp_link_;	// ����TCP����
	UdpBuffPool udp_buff_pool_;								// UDP�����
	std::mutex udp_link_lock_;								// ����UDP��ڶ������
	unsigned long udp_link_count_;						// UDP��ڼ��������ڶ��������ҵ�keyֵ
	std::map<UdpLink, UdpSocket*> udp_link_;	// ����UDP����
};

} // namespace net

#endif	// NET_RES_MANAGER_H_