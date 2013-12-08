#include "udp_socket.h"
#include <MSWSock.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

namespace net {

UdpSocket::UdpSocket() {
	callback_ = nullptr;
	socket_ = INVALID_SOCKET;
}

UdpSocket::~UdpSocket() {
  Destroy();
}

bool UdpSocket::Create(NetInterface* callback, bool broadcast) {
	if (socket_ != INVALID_SOCKET) {
		return true;
	}
	if (callback == nullptr) {
		return false;
	}
	callback_ = callback;
	socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_ == INVALID_SOCKET) {
		return false;
	}
	if (broadcast) {
		BOOL is_broadcast = TRUE;
		if (setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, (char*)&is_broadcast, sizeof(is_broadcast)) != 0) {
			Destroy();
			return false;
		}
	}
	return true;
}

bool UdpSocket::Bind(const std::string& ip, int port) {
	if (socket_ == INVALID_SOCKET) {
		return false;
	}
	SOCKADDR_IN bind_addr = { 0 };
	ToSockAddr(bind_addr, ip, port);
	if (::bind(socket_, (SOCKADDR*)&bind_addr, sizeof(bind_addr)) != 0) {
		return false;
	}
	DWORD return_bytes = 0;
	BOOL is_reset = FALSE;
	if (::WSAIoctl(socket_, SIO_UDP_CONNRESET, &is_reset, sizeof(is_reset), NULL, 0, &return_bytes, NULL, NULL) != 0) {
		return false;
	}
	return true;
}

void UdpSocket::Destroy() {
	if (socket_ != INVALID_SOCKET) {
		::closesocket(socket_);
		socket_ = INVALID_SOCKET;
		callback_ = nullptr;
	}
}

bool UdpSocket::AsyncSendTo(const char* buffer, int size, const std::string& ip, int port, LPOVERLAPPED ovlp) {
	if (socket_ == INVALID_SOCKET || buffer == nullptr || ovlp == NULL) {
		return false;
	}
  WSABUF buff = {0};
  buff.buf = const_cast<char*>(buffer);
  buff.len = size;
	DWORD bytes_sent = 0;
	SOCKADDR_IN send_to_addr = {0};
	ToSockAddr(send_to_addr, ip, port);
	if (::WSASendTo(socket_, &buff, 1, &bytes_sent, 0, (PSOCKADDR)&send_to_addr, sizeof(send_to_addr), ovlp, NULL) != 0) {
		if (::WSAGetLastError() != ERROR_IO_PENDING) {
			return false;
		}
	}
	return true;
}

bool UdpSocket::AsyncRecvFrom(char* buffer, int size, LPOVERLAPPED ovlp, PSOCKADDR_IN addr) {
	if (socket_ == INVALID_SOCKET || ovlp == NULL) {
		return false;
	}
  WSABUF buff = {0};
  buff.buf = buffer;
  buff.len = size;
	DWORD received_flag = 0;
	DWORD bytes_received = 0;
	INT addr_size = sizeof(*addr);
	if (::WSARecvFrom(socket_, &buff, 1, &bytes_received, &received_flag, (PSOCKADDR)addr, &addr_size, ovlp, NULL) != 0) {
		if (::WSAGetLastError() != ERROR_IO_PENDING) {
			return false;
		}
	}
	return true;
}

void UdpSocket::ToSockAddr(SOCKADDR_IN& addr, const std::string& ip, int port) {
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
	addr.sin_port = ::htons(static_cast<u_short>(port));
}

void UdpSocket::FromSockAddr(const SOCKADDR_IN& addr, std::string& ip, int& port) {
	ip = ::inet_ntoa(addr.sin_addr);
	port = ::ntohs(addr.sin_port);
}

} // namespace net