// ������TCP����࣬�Լ����������
// ���̣߳�֧�֣����Ƕ��߳�Ͷ�ݵ��첽����ִ��˳��δ����
// Author: cauloda
// Date: 2013-3-18

#ifndef NET_TCPSOCKET_H_
#define NET_TCPSOCKET_H_

#include "tcp_sock_attr.h"

class NetInterface;

class TcpSocket {
 public:
	TcpSocket(NetInterface* callback);
	~TcpSocket();
	bool Create();
	void Destroy();
	bool Bind(std::string ip, int port);
	bool Listen();
	bool Connect(std::string ip, int port);
	bool AsyncAccept(SOCKET listen_sock, PVOID buffer, LPOVERLAPPED ovlp);
	bool AsyncSend(char* buffer, int size, LPOVERLAPPED ovlp);
	bool AsyncRecv(char* buffer, int size, LPOVERLAPPED ovlp);
	bool SetAccepted();
	
	SOCKET socket() { return socket_; }
	NetInterface* callback() { return callback_; }
	const TcpSocketAttr& attribute() { return attribute_; }

 private:
	TcpSocket(const TcpSocket&);	// ���ɿ��������ɸ�ֵ
	TcpSocket& operator=(const TcpSocket&);
	void ResetMember();
	bool ToSockAddr(SOCKADDR_IN& addr, std::string ip, int port);
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
};

#endif	// NET_TCPSOCKET_H_