#include "tcp_sock_attr.h"
#include <Windows.h>

namespace net {

TcpSocketAttr::TcpSocketAttr() {
  send_byte_ = 0;
  recv_byte_ = 0;
  send_begin_time_ = 0;
  recv_begin_time_ = 0;
  local_port_ = 0;
  remote_port_ = 0;
}

TcpSocketAttr::~TcpSocketAttr() {
}

void TcpSocketAttr::SetBeginTime() {
  send_begin_time_ = ::GetTickCount();
  recv_begin_time_ = send_begin_time_;
}

void TcpSocketAttr::SetLocalAddr(std::string ip, int port) {
  local_ip_ = ip;
  local_port_ = port;
}

void TcpSocketAttr::SetRemoteAddr(std::string ip, int port) {
  remote_ip_ = ip;
  remote_port_ = port;
}

void TcpSocketAttr::GetLocalAddr(std::string& ip, int& port) {
  ip = local_ip_;
  port = local_port_;
}

void TcpSocketAttr::GetRemoteAddr(std::string& ip, int& port) {
  ip = remote_ip_;
  port = remote_port_;
}

void TcpSocketAttr::AddSendByte(int value) {
  std::lock_guard<std::mutex> lock(lock_);
  send_byte_ += value;
}

void TcpSocketAttr::AddRecvByte(int value) {
  std::lock_guard<std::mutex> lock(lock_);
  recv_byte_ += value;
}

int TcpSocketAttr::GetSendSpeed() {
  DWORD time_now = ::GetTickCount();
  if (time_now == send_begin_time_) {
    return 0;
  }
  int speed = static_cast<int>(send_byte_/(double)(time_now-send_begin_time_)*1000);
  std::lock_guard<std::mutex> lock(lock_);
  send_begin_time_ = time_now;
  send_byte_ = 0;
	return true;
}

int TcpSocketAttr::GetRecvSpeed() {
  DWORD time_now = ::GetTickCount();
  if (time_now == recv_begin_time_) {
    return 0;
  }
  int speed = static_cast<int>(recv_byte_/(double)(time_now-recv_begin_time_)*1000);
  std::lock_guard<std::mutex> lock(lock_);
  recv_begin_time_ = time_now;
  recv_byte_ = 0;
	return speed;
}

} // namespace net