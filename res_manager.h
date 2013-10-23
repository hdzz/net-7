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
	// 以下函数都是接口的实现，具体工作方式见实现注释
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
	ResManager(const ResManager&);	// 单例：不可拷贝，不可赋值
	ResManager& operator=(const ResManager&);
  
	int GetProcessorNum();                  // 获取处理器个数
	bool BindToIOCP(TcpSocket* socket);     // 绑定到IO完成端口
  bool IocpWorker();                      // IOCP工作线程
	TcpLink AddTcpLink(TcpSocket* socket);  // 增加一个TCP对象
	bool RemoveTcpLink(TcpLink link);       // 移除一个TCP对象
	TcpSocket* FindTcpSocket(TcpLink link); // 查找一个TCP对象

  // 异步投递IO
	bool AsyncMoreAccept(TcpLink link, TcpSocket* socket, int count);
	bool AsyncAccept(TcpLink link, TcpSocket* socket, AcceptBuff* buffer);
	bool AsyncTcpRecv(TcpLink link, TcpSocket* socket, TcpRecvBuff* buffer);
	
	// 当异步IO完成时，根据不同类型分发到不同的函数处理，具体注释见实现
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
	bool is_net_started_;								      // 网络是否已经初始化
	HANDLE iocp_;												      // IOCP内核对象句柄
	std::vector<std::thread*> iocp_thread_;   // 工作线程句柄
  unsigned long tcp_link_count_;			      // TCP插口计数，用于二叉树查找的key值
	std::map<TcpLink, TcpSocket*> tcp_link_;	// 所有TCP插口对象
	std::mutex tcp_link_lock_;                // 操作TCP插口对象的锁
  TcpBuffPool tcp_buff_pool_;               // TCP缓冲池
};

} // namespace net

#endif	// NET_RES_MANAGER_H_