#include "tcp_socket.h"
#include <MSWSock.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

namespace net {

TcpSocket::TcpSocket() {
	ResetMember();
}

TcpSocket::~TcpSocket() {
  Destroy();
}

void TcpSocket::ResetMember() {
	callback_ = nullptr;
	socket_ = INVALID_SOCKET;
	is_bind_ = false;
	is_listen_ = false;
	is_connect_ = false;
	current_header_.clear();
	current_packet_.clear();
	all_packets_.clear();
}

bool TcpSocket::Create(NetInterface* callback) {
	if (socket_ != INVALID_SOCKET) {
		return true;
	}
	if (callback == nullptr) {
		return false;
	}
	callback_ = callback;
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

bool TcpSocket::Bind(const std::string& ip, int port) {
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
	ToSockAddr(bind_addr, ip, port);
	if (::bind(socket_, (SOCKADDR*)&bind_addr, sizeof(bind_addr)) != 0) {
		return false;
	}
	is_bind_ = true;
	return true;
}

bool TcpSocket::Listen(int backlog) {
	if (socket_ == INVALID_SOCKET || !is_bind_) {
		return false;
	}
	if (is_listen_) {
		return true;
	}
	if (::listen(socket_, backlog) != 0) {
		return false;
	}
	is_listen_ = true;
	return true;
}

bool TcpSocket::Connect(const std::string& ip, int port) {
	if (socket_ == INVALID_SOCKET || !is_bind_ || is_listen_) {
		return false;
	}
	if (is_connect_) {
		return true;
	}
	SOCKADDR_IN connect_addr = {0};
	ToSockAddr(connect_addr, ip, port);
	if (::connect(socket_, (SOCKADDR*)&connect_addr, sizeof(connect_addr)) != 0) {
		return false;
	}
	is_connect_ = true;
	return true;
}

bool TcpSocket::AsyncAccept(SOCKET accept_sock, char* buffer, LPOVERLAPPED ovlp) {
	if (buffer == nullptr || ovlp == NULL){
		return false;
	}
	int addr_size = sizeof(SOCKADDR_IN) + 16;
	DWORD bytes_received = 0;
	if (!::AcceptEx(socket_, accept_sock, buffer, 0, addr_size, addr_size, &bytes_received, ovlp)) {
		if (::WSAGetLastError() != ERROR_IO_PENDING) {
			return false;
		}
	}
	return true;
}

bool TcpSocket::AsyncSend(const char* buffer, int size, LPOVERLAPPED ovlp) {
	if (!is_connect_ || buffer == nullptr || ovlp == NULL) {
		return false;
	}
  WSABUF buff = {0};
  buff.buf = const_cast<char*>(buffer);
  buff.len = size;
	DWORD bytes_sent = 0;
	if (::WSASend(socket_, &buff, 1, &bytes_sent, 0, ovlp, NULL) != 0) {
		if (::WSAGetLastError() != ERROR_IO_PENDING) {
			return false;
		}
	}
	return true;
}

bool TcpSocket::AsyncRecv(char* buffer, int size, LPOVERLAPPED ovlp) {
	if ((!is_listen_ && !is_connect_) || ovlp == NULL) {
		return false;
	}
  WSABUF buff = {0};
  buff.buf = buffer;
  buff.len = size;
	DWORD received_flag = 0;
	DWORD bytes_received = 0;
	if (::WSARecv(socket_, &buff, 1, &bytes_received, &received_flag, ovlp, NULL) != 0) {
		if (::WSAGetLastError() != ERROR_IO_PENDING) {
			return false;
		}
	}
	return true;
}

bool TcpSocket::SetAccepted(SOCKET listen_sock) {
	if (0 != ::setsockopt(socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listen_sock, sizeof(listen_sock))) {
		return false;
	}
	is_bind_ = true;
	is_connect_ = true;
	return true;
}

bool TcpSocket::GetLocalAddr(std::string& ip, int& port) {
	SOCKADDR_IN addr = {0};
	int size = sizeof(addr);
	if (::getsockname(socket_, (SOCKADDR*)&addr, &size) != 0) {
		return false;
	}
	FromSockAddr(addr, ip, port);
	return true;
}

bool TcpSocket::GetRemoteAddr(std::string& ip, int& port) {
	SOCKADDR_IN addr = {0};
	int size = sizeof(addr);
	if (::getpeername(socket_, (SOCKADDR*)&addr, &size) != 0) {
		return false;
	}
	FromSockAddr(addr, ip, port);
	return true;
}

void TcpSocket::ToSockAddr(SOCKADDR_IN& addr, const std::string& ip, int port) {
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
	addr.sin_port = ::htons(static_cast<u_short>(port));
}

void TcpSocket::FromSockAddr(const SOCKADDR_IN& addr, std::string& ip, int& port) {
	ip = ::inet_ntoa(addr.sin_addr);
	port = ::ntohs(addr.sin_port);
}

} // namespace net