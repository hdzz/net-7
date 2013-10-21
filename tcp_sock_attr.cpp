#include "tcp_sock_attr.h"

TcpSocketAttr::TcpSocketAttr() {
	send_byte_ = 0;
	recv_byte_ = 0;
	send_time_ = 0;
	recv_time_ = 0;
	local_port_ = 0;
	remote_port_ = 0;
	packet_count_ = 0;
	last_recv_pkt_ = 0;
	last_recv_buff_ = -1;
}

TcpSocketAttr::~TcpSocketAttr() {
		for (auto i : recving_pkt_) {
			delete i.second;
		}
}

void TcpSocketAttr::SetBeginTime() {
	send_time_ = ::GetTickCount();
	recv_time_ = send_time_;
}

void TcpSocketAttr::SetLocalAddr(string ip, int port) {
	local_ip_ = ip;
	local_port_ = port;
}

void TcpSocketAttr::SetRemoteAddr(string ip, int port) {
	remote_ip_ = ip;
	remote_port_ = port;
}

void TcpSocketAttr::GetLocalAddr(string& ip, int& port) {
	ip = local_ip_;
	port = local_port_;
}

void TcpSocketAttr::GetRemoteAddr(string& ip, int& port) {
	ip = remote_ip_;
	port = remote_port_;
}

void TcpSocketAttr::AddSendByte(int value) {
	ThreadLock<CriticalSection> lock(lock_);
	send_byte_ += value;
}

void TcpSocketAttr::AddRecvByte(int value) {
	ThreadLock<CriticalSection> lock(lock_);
	recv_byte_ += value;
}

int TcpSocketAttr::GetSendSpeed() {
	DWORD time_now = ::GetTickCount();
	if (time_now == send_time_) {
		return 0;
	}
	int speed = static_cast<int>(send_byte_/(double)(time_now-send_time_)*1000);
	ThreadLock<CriticalSection> lock(lock_);
	send_time_ = time_now;
	send_byte_ = 0;
	return true;
}

int TcpSocketAttr::GetRecvSpeed() {
	DWORD time_now = ::GetTickCount();
	if (time_now == recv_time_) {
		return 0;
	}
	int speed = static_cast<int>(recv_byte_/(double)(time_now-recv_time_)*1000);
	ThreadLock<CriticalSection> lock(lock_);
	recv_time_ = time_now;
	recv_byte_ = 0;
	return speed;
}

int TcpSocketAttr::AddPacketCount() {
	ThreadLock<CriticalSection> lock(lock_);
	++packet_count_;
	return packet_count_;
}