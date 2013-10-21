#include "tcp_socket.h"
#include <MSWSock.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

TcpSocket::TcpSocket(NetInterface* callback) {
	ResetMember();
	callback_ = callback;
}

TcpSocket::~TcpSocket() { Destroy(); }

void TcpSocket::ResetMember() {
	callback_ = nullptr;
	socket_ = INVALID_SOCKET;
	listen_sock_ = INVALID_SOCKET;
	is_bind_ = false;
	is_listen_ = false;
	is_connect_ = false;
}

bool TcpSocket::Create() {
	if (socket_ != INVALID_SOCKET) {
		return true;
	}
	socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_ == INVALID_SOCKET) {
		return false;
	}
	return true;
}

void TcpSocket::Destroy() {
	if (socket_ != INVALID_SOCKET) {
		::shutdown(socket_, SD_SEND);
		::closesocket(socket_);
		ResetMember();
	}
}

bool TcpSocket::Bind(std::string ip, int port) {
	if (socket_ == INVALID_SOCKET) {
		return false;
	}
	if (is_bind_) {
		return true;
	}
	auto no_delay = TRUE;
	if (::setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char*)&no_delay, sizeof(no_delay)) != 0) {
		return false;
	}
	auto reuse_addr = FALSE;
	if (::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_addr, sizeof(reuse_addr)) != 0) {
		return false;
	}
	SOCKADDR_IN bind_addr = {0};
	if (!ToSockAddr(bind_addr, ip, port)) {
		return false;
	}
	if (::bind(socket_, (SOCKADDR*)&bind_addr, sizeof(bind_addr)) != 0) {
		return false;
	}
	attribute_.SetLocalAddr(ip, port);
	is_bind_ = true;
	return true;
}

bool TcpSocket::Listen() {
	if (socket_ == INVALID_SOCKET || !is_bind_) {
		return false;
	}
	if (is_listen_) {
		return true;
	}
	if (::listen(socket_, SOMAXCONN) != 0) {
		return false;
	}
	is_listen_ = true;
	return true;
}

bool TcpSocket::Connect(std::string ip, int port) {
	if (socket_ == INVALID_SOCKET || !is_bind_ || is_listen_) {
		return false;
	}
	if (is_connect_) {
		return true;
	}
	SOCKADDR_IN connect_addr = {0};
	if (!ToSockAddr(connect_addr, ip, port)) {
		return false;
	}
	if (::connect(socket_, (SOCKADDR*)&connect_addr, sizeof(connect_addr)) != 0) {
		return false;
	}
	attribute_.SetRemoteAddr(ip, port);
	attribute_.SetBeginTime();
	is_connect_ = true;
	return true;
}

bool TcpSocket::AsyncAccept(SOCKET listening_socket, PVOID buffer, LPOVERLAPPED ovlp) {
	if (listening_socket == INVALID_SOCKET || !buffer || !ovlp){
		return false;
	}
	listen_sock_ = listening_socket;
	int addr_size = sizeof(SOCKADDR_IN) + 16;
	DWORD bytes_received = 0;
	if (!::AcceptEx(listen_sock_, socket_, buffer, 0, addr_size, addr_size, &bytes_received, ovlp)) {
		if (WSAGetLastError() != ERROR_IO_PENDING) {
			return false;
		}
	}
	return true;
}

bool TcpSocket::AsyncSend(char* buffer, int size, LPOVERLAPPED ovlp) {
	if (!is_connect_ || buffer == nullptr || ovlp == NULL) {
		return false;
	}
	WSABUF wsa_buffer[1] = {0};
	wsa_buffer[0].buf = buffer;
	wsa_buffer[0].len = size;
	DWORD bytes_sent = 0;
	if (::WSASend(socket_, wsa_buffer, 1, &bytes_sent, 0, ovlp, NULL) != 0) {
		if (WSAGetLastError() != ERROR_IO_PENDING) {
			return false;
		}
	}
	return true;
}

bool TcpSocket::AsyncRecv(char* buffer, int size, LPOVERLAPPED ovlp) {
	if ((!is_listen_ && !is_connect_) || !buffer || !ovlp) {
		return false;
	}
	WSABUF wsa_buffer[1] = {0};
	wsa_buffer[0].buf = buffer;
	wsa_buffer[0].len = size;
	DWORD received_flag = 0;
	DWORD bytes_received = 0;
	if (::WSARecv(socket_, wsa_buffer, 1, &bytes_received, &received_flag, ovlp, NULL) != 0) {
		if (WSAGetLastError() != ERROR_IO_PENDING) {
			return false;
		}
	}
	return true;
}

bool TcpSocket::SetAccepted() {
	if (0 != ::setsockopt(socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listen_sock_, sizeof(listen_sock_))) {
			return false;
	}
	string local_ip, remote_ip;
	int local_port, remote_port;
	GetLocalAddr(local_ip, local_port);
	GetRemoteAddr(remote_ip, remote_port);
	attribute_.SetLocalAddr(local_ip, local_port);
	attribute_.SetRemoteAddr(remote_ip, remote_port);
	attribute_.SetBeginTime();
	is_connect_ = true;
	return true;
}

bool TcpSocket::ToSockAddr(SOCKADDR_IN& addr, string ip, int port) {
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
	addr.sin_port = ::htons(static_cast<u_short>(port));
	return true;
}

bool TcpSocket::FromSockAddr(const SOCKADDR_IN& addr, string& ip, int& port) {
	ip = ::inet_ntoa(addr.sin_addr);
	port = ::ntohs(addr.sin_port);
	return true;
}

bool TcpSocket::GetLocalAddr(string& ip, int& port) {
	SOCKADDR_IN addr = {0};
	int size = sizeof(addr);
	if (::getsockname(socket_, (SOCKADDR*)&addr, &size) != 0) {
		return false;
	}
	return FromSockAddr(addr, ip, port);
}

bool TcpSocket::GetRemoteAddr(string& ip, int& port) {
	SOCKADDR_IN addr = {0};
	int size = sizeof(addr);
	if (::getpeername(socket_, (SOCKADDR*)&addr, &size) != 0) {
		return false;
	}
	return FromSockAddr(addr, ip, port);
}