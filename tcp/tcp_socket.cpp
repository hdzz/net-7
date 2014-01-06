#include "tcp_socket.h"
#include "tcp_header.h"
#include "log.h"
#include "utility.h"
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
  bind_ = false;
  listen_ = false;
  connect_ = false;
  current_header_.clear();
  current_packet_.clear();
  all_packets_.clear();
  current_packet_size_ = 0;
}

bool TcpSocket::Create(NetInterface* callback) {
  if (socket_ != INVALID_SOCKET) {
    LOG(kError, "create tcp socket failed: already created.");
    return false;
  }
  if (callback == nullptr) {
    LOG(kError, "create tcp socket failed: invalid callback parameter.");
    return false;
  }
  callback_ = callback;
  socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_ == INVALID_SOCKET) {
    LOG(kError, "create tcp socket failed, error code: %d.", ::WSAGetLastError());
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
    LOG(kError, "bind tcp socket failed: not created.");
    return false;
  }
  if (bind_) {
    LOG(kError, "bind tcp socket failed: already bound.");
    return false;
  }
  auto no_delay = TRUE;
  if (::setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (const char*)&no_delay, sizeof(no_delay)) != 0) {
    LOG(kError, "set tcp socket nodelay option failed, error code: %d.", ::WSAGetLastError());
    return false;
  }
  auto reuse_addr = TRUE;
  if (::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_addr, sizeof(reuse_addr)) != 0) {
    LOG(kError, "set tcp socket reuse address option failed, error code: %d.", ::WSAGetLastError());
    return false;
  }
  SOCKADDR_IN bind_addr = {0};
  utility::ToSockAddr(bind_addr, ip, port);
  if (::bind(socket_, (SOCKADDR*)&bind_addr, sizeof(bind_addr)) != 0) {
    LOG(kError, "bind tcp socket failed, error code: %d.", ::WSAGetLastError());
    return false;
  }
  bind_ = true;
  return true;
}

bool TcpSocket::Listen(int backlog) {
  if (socket_ == INVALID_SOCKET || !bind_) {
    LOG(kError, "listen tcp socket failed: not created or not bind.");
    return false;
  }
  if (listen_) {
    LOG(kError, "listen tcp socket failed: already listened.");
    return false;
  }
  if (::listen(socket_, backlog) != 0) {
    LOG(kError, "listen tcp socket failed, error code: %d.", ::WSAGetLastError());
    return false;
  }
  listen_ = true;
  return true;
}

bool TcpSocket::Connect(const std::string& ip, int port) {
  if (socket_ == INVALID_SOCKET || !bind_ || listen_) {
    LOG(kError, "connect tcp socket failed: not created or not bind or is listening.");
    return false;
  }
  if (connect_) {
    LOG(kError, "connect tcp socket failed: already connected.");
    return false;
  }
  SOCKADDR_IN connect_addr = {0};
  utility::ToSockAddr(connect_addr, ip, port);
  if (::connect(socket_, (SOCKADDR*)&connect_addr, sizeof(connect_addr)) != 0) {
    LOG(kError, "connect tcp socket failed, error code: %d.", ::WSAGetLastError());
    return false;
  }
  connect_ = true;
  return true;
}

bool TcpSocket::AsyncAccept(SOCKET accept_sock, char* buffer, LPOVERLAPPED ovlp) {
  if (socket_ == INVALID_SOCKET) {
    LOG(kError, "async tcp socket accept buffer failed: not created.");
    return false;
  }
  if (accept_sock == INVALID_SOCKET || buffer == nullptr || ovlp == NULL){
    LOG(kError, "async tcp socket accept buffer failed: invalid parameter.");
    return false;
  }
  auto addr_size = sizeof(SOCKADDR_IN)+16;
  DWORD bytes_received = 0;
  if (!::AcceptEx(socket_, accept_sock, buffer, 0, addr_size, addr_size, &bytes_received, ovlp)) {
    if (::WSAGetLastError() != ERROR_IO_PENDING) {
      LOG(kError, "AcceptEx failed, error code: %d.", ::WSAGetLastError());
      return false;
    }
  }
  return true;
}

bool TcpSocket::AsyncSend(const char* buffer, int size, LPOVERLAPPED ovlp) {
  if (socket_ == INVALID_SOCKET) {
    LOG(kError, "async tcp socket send buffer failed: not created.");
    return false;
  }
  if (!connect_) {
    LOG(kError, "async tcp socket send buffer failed: not connected.");
    return false;
  }
  if (buffer == nullptr || size == 0 || ovlp == NULL) {
    LOG(kError, "async tcp socket send buffer failed: invalid parameter.");
    return false;
  }
  WSABUF buff = {0};
  buff.buf = const_cast<char*>(buffer);
  buff.len = size;
  if (::WSASend(socket_, &buff, 1, NULL, 0, ovlp, NULL) != 0) {
    if (::WSAGetLastError() != ERROR_IO_PENDING) {
      LOG(kError, "WSASend failed, error code: %d.", ::WSAGetLastError());
      return false;
    }
  }
  return true;
}

bool TcpSocket::AsyncRecv(char* buffer, int size, LPOVERLAPPED ovlp) {
  if (socket_ == INVALID_SOCKET) {
    LOG(kError, "async tcp socket recv buffer failed: not created.");
    return false;
  }
  if (!connect_) {
    LOG(kError, "async tcp socket recv buffer failed: not connected.");
    return false;
  }
  if (buffer == nullptr || size == 0 || ovlp == NULL) {
    LOG(kError, "async tcp socket recv buffer failed: invalid parameter.");
    return false;
  }
  WSABUF buff = {0};
  buff.buf = buffer;
  buff.len = size;
  DWORD received_flag = 0;
  if (::WSARecv(socket_, &buff, 1, NULL, &received_flag, ovlp, NULL) != 0) {
    if (::WSAGetLastError() != ERROR_IO_PENDING) {
      LOG(kError, "WSARecv failed, error code: %d.", ::WSAGetLastError());
      return false;
    }
  }
  return true;
}

bool TcpSocket::SetAccepted(SOCKET listen_sock) {
  if (socket_ == INVALID_SOCKET) {
    LOG(kError, "set tcp socket accept context failed: not created.");
    return false;
  }
  if (listen_sock == INVALID_SOCKET) {
    LOG(kError, "set tcp socket accept context failed: invalid parameter.");
    return false;
  }
  if (0 != ::setsockopt(socket_, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listen_sock, sizeof(listen_sock))) {
    LOG(kError, "set tcp socket accept context failed, error code: %d.", ::WSAGetLastError());
    return false;
  }
  bind_ = true;
  connect_ = true;
  return true;
}

bool TcpSocket::GetLocalAddr(std::string& ip, int& port) {
  SOCKADDR_IN addr = {0};
  int size = sizeof(addr);
  if (::getsockname(socket_, (SOCKADDR*)&addr, &size) != 0) {
    LOG(kError, "getsockname failed, error code: %d.", ::WSAGetLastError());
    return false;
  }
  utility::FromSockAddr(addr, ip, port);
  return true;
}

bool TcpSocket::GetRemoteAddr(std::string& ip, int& port) {
  SOCKADDR_IN addr = {0};
  int size = sizeof(addr);
  if (::getpeername(socket_, (SOCKADDR*)&addr, &size) != 0) {
    LOG(kError, "getsockname failed, error code: %d.", ::WSAGetLastError());
    return false;
  }
  utility::FromSockAddr(addr, ip, port);
  return true;
}

bool TcpSocket::OnRecv(const char* data, int size) {
  if (data == nullptr || size == 0) {
    return false;
  }
  auto total_dealed = 0;
  while (total_dealed < size) {
    auto current_dealed = 0;
    if (!ParseTcpHeader(&data[total_dealed], size - total_dealed, current_dealed)) {
      return false;
    }
    total_dealed += current_dealed;
    if (total_dealed >= size) {
      break;
    }
    current_dealed = ParseTcpPacket(&data[total_dealed], size - total_dealed);
    total_dealed += current_dealed;
  }
  return true;
}

bool TcpSocket::CalcPacketSize() {
  if (!current_header_.empty()) {
    TcpHeader header;
    if (!header.Init(&current_header_[0], current_header_.size())) {
      return false;
    }
    current_packet_size_ = header.packet_size();
    return true;
  }
  return false;
}

bool TcpSocket::ParseTcpHeader(const char* data, int size, int& dealed_size) {
  if (current_header_.size() < kTcpHeaderSize) {
    int left_header_size = kTcpHeaderSize - current_header_.size();
    if (left_header_size > size) {
      for (auto i = 0; i < size; ++i) {
        current_header_.push_back(data[i]);
      }
      dealed_size = size;
    } else {
      for (auto i = 0; i < left_header_size; ++i) {
        current_header_.push_back(data[i]);
      }
      dealed_size = left_header_size;
      current_packet_.clear();
      if (!CalcPacketSize()) {
        return false;
      }
      current_packet_.reserve(current_packet_size_);
    }
  }
  return true;
}

int TcpSocket::ParseTcpPacket(const char* data, int size) {
  int left_packet_size = current_packet_size_ - current_packet_.size();
  if (left_packet_size > 0) {
    if (left_packet_size > size) {
      for (auto i = 0; i < size; ++i) {
        current_packet_.push_back(data[i]);
      }
      return size;
    } else {
      for (auto i = 0; i < left_packet_size; ++i) {
        current_packet_.push_back(data[i]);
      }
      all_packets_.push_back(std::move(current_packet_));
      current_packet_size_ = 0;
      current_header_.clear();
      return left_packet_size;
    }
  }
  return 0;
}

} // namespace net