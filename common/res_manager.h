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
	// 以下函数都是接口的实现，具体工作方式见实现注释
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
	ResManager(const ResManager&);	// 单例：不可拷贝，不可赋值
	ResManager& operator=(const ResManager&);
  
	TcpLink AddTcpLink(TcpSocket* new_socket);	// 增加一个TCP对象
	void RemoveTcpLink(TcpLink link);						// 移除一个TCP对象
	TcpSocket* FindTcpSocket(TcpLink link);			// 查找一个TCP对象
	UdpLink AddUdpLink(UdpSocket* new_socket);	// 增加一个UDP对象
	void RemoveUdpLink(UdpLink link);						// 移除一个UDP对象
	UdpSocket* FindUdpSocket(UdpLink link);			// 查找一个UDP对象

  // 异步投递IO
	bool AsyncMoreAccept(TcpLink link, TcpSocket* socket, int count);
	bool AsyncAccept(TcpLink link, TcpSocket* socket, AcceptBuff* buffer);
	bool AsyncTcpRecv(TcpLink link, TcpSocket* socket, TcpRecvBuff* buffer);
	void OnTcpRecvNothing(TcpLink link, TcpRecvBuff* buffer, NetInterface* callback);
	void OnTcpRecvError(TcpLink link, TcpRecvBuff* buffer, NetInterface* callback);
	bool AsyncMoreUdpRecv(UdpLink link, UdpSocket* socket, int count);
	bool AsyncUdpRecv(UdpLink link, UdpSocket* socket, UdpRecvBuff* buffer);
	void OnUdpRecvError(UdpLink link, UdpRecvBuff* buffer, NetInterface* callback);
	
	// 当异步IO完成时，根据不同类型分发到不同的函数处理，具体注释见实现
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
	bool is_net_started_;											// 网络是否已经初始化
	IOCP iocp_;																// IOCP对象
	TcpBuffPool tcp_buff_pool_;								// TCP缓冲池
	std::mutex tcp_link_lock_;								// 操作TCP插口对象的锁
  unsigned long tcp_link_count_;						// TCP插口计数，用于二叉树查找的key值
	std::map<TcpLink, TcpSocket*> tcp_link_;	// 所有TCP对象
	UdpBuffPool udp_buff_pool_;								// UDP缓冲池
	std::mutex udp_link_lock_;								// 操作UDP插口对象的锁
	unsigned long udp_link_count_;						// UDP插口计数，用于二叉树查找的key值
	std::map<UdpLink, UdpSocket*> udp_link_;	// 所有UDP对象
};

} // namespace net

#endif	// NET_RES_MANAGER_H_