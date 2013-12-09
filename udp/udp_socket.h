#ifndef NET_UDP_SOCKET_H_
#define NET_UDP_SOCKET_H_

#include <mutex>
#include <string>
#include <vector>
#include <WinSock2.h>

namespace net {

class NetInterface;

class UdpSocket {
 public:
	UdpSocket();
	~UdpSocket();

	bool Create(NetInterface* callback, bool broadcast);
	bool Bind(const std::string& ip, int port);
	void Destroy();
	bool AsyncSendTo(const char* buffer, int size, const std::string& ip, int port, LPOVERLAPPED ovlp);
	bool AsyncRecvFrom(char* buffer, int size, LPOVERLAPPED ovlp, PSOCKADDR_IN addr, PINT addr_size);
	
	void ToSockAddr(SOCKADDR_IN& addr, const std::string& ip, int port);
	void FromSockAddr(const SOCKADDR_IN& addr, std::string& ip, int& port);

	SOCKET socket() { return socket_; }
	NetInterface* callback() { return callback_; }

 private:
	UdpSocket(const UdpSocket&);	// 不可拷贝，不可赋值
	UdpSocket& operator=(const UdpSocket&);

 private:
	NetInterface* callback_;
	SOCKET socket_;
};

} // namespace net

#endif	// NET_UDP_SOCKET_H_