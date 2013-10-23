#ifndef NET_TCP_SOCKET_H_
#define NET_TCP_SOCKET_H_

#include "tcp_sock_attr.h"
#include <vector>
#include <WinSock2.h>

namespace net {

class NetInterface;

class TcpSocket {
 public:
	TcpSocket(NetInterface* callback);
	~TcpSocket();
	bool Create();
	void Destroy();
	bool Bind(const std::string& ip, int port);
	bool Listen(int backlog);
	bool Connect(const std::string& ip, int port);
	bool AsyncAccept(SOCKET listen_sock, char* buffer, LPOVERLAPPED ovlp);
	bool AsyncSend(const char* buffer, int size, LPOVERLAPPED ovlp);
	bool AsyncRecv(const char* buffer, int size, LPOVERLAPPED ovlp);
	bool SetAccepted();
	
	SOCKET socket() { return socket_; }
	NetInterface* callback() { return callback_; }
	TcpSocketAttr& attribute() { return attribute_; }
  std::vector<char>& current_header() { return current_header_; }
  std::vector<char>& current_packet() { return current_packet_; }
  std::vector<std::vector<char>>& all_packets() { return all_packets_; }

 private:
	TcpSocket(const TcpSocket&);	// 不可拷贝，不可赋值
	TcpSocket& operator=(const TcpSocket&);
	void ResetMember();
	bool ToSockAddr(SOCKADDR_IN& addr, const std::string& ip, int port);
	bool FromSockAddr(const SOCKADDR_IN& addr, std::string& ip, int& port);
	bool GetLocalAddr(std::string& ip, int& port);
	bool GetRemoteAddr(std::string& ip, int& port);

 private:
	NetInterface* callback_;
	SOCKET socket_;
	SOCKET listen_sock_;
	TcpSocketAttr attribute_;
	bool is_bind_;
	bool is_listen_;
	bool is_connect_;
  std::vector<char> current_header_;  // 当前在处理的TCP头 
  std::vector<char> current_packet_;  // 当前在处理的包
  std::vector<std::vector<char>> all_packets_;  // 需要回调的所有包
};

} // namespace net

#endif	// NET_TCP_SOCKET_H_