#include "res_manager.h"
#include "log.h"
#include "utility.h"

namespace net {

const TcpHandle kMaxTcpHandleNumber = 0xFFFFFFFF;
const UdpHandle kMaxUdpHandleNumber = 0xFFFFFFFF;

ResManager::ResManager() {
  net_started_ = false;
  tcp_socket_count_ = 0;
  udp_socket_count_ = 0;
}

ResManager::~ResManager() {
  CleanupNet();
}

bool ResManager::StartupNet() {
  if (net_started_) {
    return true;
  }
  net_started_ = true;
  tcp_buff_pool_.Init();
  udp_buff_pool_.Init();
  auto iocp_callback = std::bind(&ResManager::TransferAsyncType, this, std::placeholders::_1, std::placeholders::_2);
  if (!iocp_.Init(iocp_callback)) {
    CleanupNet();
    return false;
  }
  return true;
}

bool ResManager::CleanupNet() {
  if (!net_started_) {
    return true;
  }
  tcp_socket_lock_.lock();
  tcp_socket_.clear();
  tcp_socket_count_ = 0;
  tcp_socket_lock_.unlock();
  udp_socket_lock_.lock();
  udp_socket_.clear();
  udp_socket_count_ = 0;
  udp_socket_lock_.unlock();
  iocp_.Uninit();
  tcp_buff_pool_.Clear();
  udp_buff_pool_.Clear();
  net_started_ = false;
  return true;
}

bool ResManager::TcpCreate(NetInterface* callback, const char* ip, int port, TcpHandle& new_handle) {
  if (callback == nullptr) {
    LOG(kError, "create tcp handle failed: invalid callback parameter.");
    return false;
  }
  if (ip == nullptr) {
    ip = "0.0.0.0";
  }
  std::shared_ptr<TcpSocket> new_socket(new TcpSocket);
  if (!new_socket) {
    LOG(kError, "create tcp handle failed: not enough memory.");
    return false;
  }
  if (!new_socket->Create(callback)) {
    return false;
  }
  if (!new_socket->Bind(ip, port)) {
    return false;
  }
  if (!iocp_.BindToIOCP(new_socket->socket())) {
    return false;
  }
  if (!NewTcpSocket(new_handle, new_socket)) {
    return false;
  }
  return true;
}

bool ResManager::TcpDestroy(TcpHandle handle) {
  RemoveTcpSocket(handle);
  return true;
}

bool ResManager::TcpListen(TcpHandle handle) {
  auto socket = GetTcpSocket(handle);
  if (!socket) {
    LOG(kError, "listen tcp handle: %u failed: no such handle.", handle);
    return false;
  }
  auto backlog = utility::GetProcessorNum() * 2;
  if (!socket->Listen(backlog)) {
    return false;
  }
  if (!AsyncMoreTcpAccept(handle, socket, backlog)) {
    return false;
  }
  return true;
}

bool ResManager::TcpConnect(TcpHandle handle, const char* ip, int port) {
  auto socket = GetTcpSocket(handle);
  if (!socket) {
    LOG(kError, "connect tcp handle: %u failed: no such handle.", handle);
    return false;
  }
  if (!socket->Connect(ip, port)) {
    return false;
  }
  auto recv_buffer = tcp_buff_pool_.GetRecvBuffer();
  if (recv_buffer == nullptr) {
    LOG(kError, "connect tcp handle: %u failed: can not find one recv buffer.", handle);
    return false;
  }
  if (!AsyncTcpRecv(handle, socket, recv_buffer)) {
    return false;
  }
  return true;
}

// todo: consider whether buffer pool is reasonable, and send more than one buffer once
bool ResManager::TcpSend(TcpHandle handle, const char* packet, int size) {
  if (packet == nullptr || size <= 0 || size > kMaxTcpPacketSize) {
    LOG(kError, "send tcp handle: %u packet failed: invalid parameter.", handle);
    return false;
  }
  auto socket = GetTcpSocket(handle);
  if (!socket) {
    LOG(kError, "send tcp handle: %u packet failed: no such handle.", handle);
    return false;
  }
  auto send_failed = false;
  auto send_buffer = tcp_buff_pool_.GetSendBuffer(packet, size);
  for (auto& i : send_buffer) {
    if (!send_failed) {
      i->set_handle(handle);
      if (!socket->AsyncSend(i->buffer(), i->buffer_size(), i->ovlp())) {
        tcp_buff_pool_.ReturnSendBuff(i);
        send_failed = true;
      }
    } else {
      tcp_buff_pool_.ReturnSendBuff(i);
    }
  }
  if (send_failed) {
    return false;
  } else {
    return true;
  }
}

bool ResManager::TcpGetLocalAddr(TcpHandle handle, char ip[16], int& port) {
  if (ip == nullptr) {
    LOG(kError, "get tcp handle : %u local address failed: invalid ip parameter.", handle);
    return false;
  }
  auto socket = GetTcpSocket(handle);
  if (!socket) {
    LOG(kError, "get tcp handle : %u local address failed: no such handle.", handle);
    return false;
  }
  std::string ip_string;
  if (!socket->GetLocalAddr(ip_string, port)) {
    return false;
  }
  strcpy_s(ip, 16, ip_string.c_str());
  return true;
}

bool ResManager::TcpGetRemoteAddr(TcpHandle handle, char ip[16], int& port) {
  if (ip == nullptr) {
    LOG(kError, "get tcp handle : %u remote address failed: invalid ip parameter.", handle);
    return false;
  }
  auto socket = GetTcpSocket(handle);
  if (!socket) {
    LOG(kError, "get tcp handle : %u remote address failed: no such handle.", handle);
    return false;
  }
  std::string ip_string;
  if (!socket->GetRemoteAddr(ip_string, port)) {
    return false;
  }
  strcpy_s(ip, 16, ip_string.c_str());
  return true;
}

bool ResManager::UdpCreate(NetInterface* callback, const char* ip, int port, UdpHandle& new_handle) {
  if (callback == nullptr) {
    LOG(kError, "create udp handle failed: invalid callback parameter.");
    return false;
  }
  if (ip == nullptr) {
    ip = "0.0.0.0";
  }
  std::shared_ptr<UdpSocket> new_socket(new UdpSocket);
  if (!new_socket) {
    LOG(kError, "create udp handle failed: not enough memory.");
    return false;
  }
  if (!new_socket->Create(callback)) {
    return false;
  }
  if (!new_socket->Bind(ip, port)) {
    return false;
  }
  if (!iocp_.BindToIOCP(new_socket->socket())) {
    return false;
  }
  if (!NewUdpSocket(new_handle, new_socket)) {
    return false;
  }
  auto recv_count = utility::GetProcessorNum();
  if (!AsyncMoreUdpRecv(new_handle, new_socket, recv_count)) {
    RemoveUdpSocket(new_handle);
    return false;
  }
  return true;
}

bool ResManager::UdpDestroy(UdpHandle handle) {
  RemoveUdpSocket(handle);
  return true;
}

bool ResManager::UdpSendTo(UdpHandle handle, const char* packet, int size, const char* ip, int port) {
  if (packet == nullptr || size <= 0 || size > kMaxUdpPacketSize || ip == nullptr || port <= 0) {
    LOG(kError, "send udp handle: %u packet failed: invalid parameter.", handle);
    return false;
  }
  auto socket = GetUdpSocket(handle);
  if (!socket) {
    LOG(kError, "send udp handle: %u packet failed: no such handle.", handle);
    return false;
  }
  auto send_buffer = udp_buff_pool_.GetSendBuffer();
  if (send_buffer == nullptr) {
    LOG(kError, "send udp handle: %u failed: can not find one send buffer.", handle);
    return false;
  }
  send_buffer->set_buffer(packet, size);
  send_buffer->set_handle(handle);
  if (!socket->AsyncSendTo(send_buffer->buffer(), send_buffer->buffer_size(), ip, port, send_buffer->ovlp())) {
    udp_buff_pool_.ReturnSendBuff(send_buffer);
    return false;
  }
  return true;
}

bool ResManager::NewTcpSocket(TcpHandle& new_handle, const std::shared_ptr<TcpSocket>& new_socket) {
  std::lock_guard<std::mutex> lock(tcp_socket_lock_);
  if (tcp_socket_.size() == kMaxTcpHandleNumber) {
    LOG(kError, "fail to new tcp handle: reach max tcp handle number.");
    return false;
  }
  if (tcp_socket_count_ != kMaxTcpHandleNumber) {
    ++tcp_socket_count_;
    new_handle = tcp_socket_count_;
  } else {
    TcpHandle min_handle = 1;
    auto socket = tcp_socket_.find(min_handle);
    while (socket != tcp_socket_.end()) {
      ++min_handle;
      socket = tcp_socket_.find(min_handle);
    }
    new_handle = min_handle;
  }
  tcp_socket_.insert(std::make_pair(new_handle, new_socket));
  return true;
}

bool ResManager::NewUdpSocket(TcpHandle& new_handle, const std::shared_ptr<UdpSocket>& new_socket) {
  std::lock_guard<std::mutex> lock(udp_socket_lock_);
  if (udp_socket_.size() == kMaxUdpHandleNumber) {
    LOG(kError, "fail to new udp handle: reach max udp handle number.");
    return false;
  }
  if (udp_socket_count_ != kMaxUdpHandleNumber) {
    ++udp_socket_count_;
    new_handle = udp_socket_count_;
  } else {
    UdpHandle min_handle = 1;
    auto socket = udp_socket_.find(min_handle);
    while (socket != udp_socket_.end()) {
      ++min_handle;
      socket = udp_socket_.find(min_handle);
    }
    new_handle = min_handle;
  }
  udp_socket_.insert(std::make_pair(new_handle, new_socket));
  return true;
}

void ResManager::RemoveTcpSocket(TcpHandle handle) {
  std::lock_guard<std::mutex> lock(tcp_socket_lock_);
  auto socket = tcp_socket_.find(handle);
  if (socket == tcp_socket_.end()) {
    return;
  }
  tcp_socket_.erase(socket);
  return;
}

void ResManager::RemoveUdpSocket(UdpHandle handle) {
  std::lock_guard<std::mutex> lock(udp_socket_lock_);
  auto socket = udp_socket_.find(handle);
  if (socket == udp_socket_.end()) {
    return;
  }
  udp_socket_.erase(socket);
  return;
}

std::shared_ptr<TcpSocket> ResManager::GetTcpSocket(TcpHandle handle) {
  std::lock_guard<std::mutex> lock(tcp_socket_lock_);
  auto socket = tcp_socket_.find(handle);
  if (socket == tcp_socket_.end()) {
    return nullptr;
  }
  return socket->second;
}

std::shared_ptr<UdpSocket> ResManager::GetUdpSocket(UdpHandle handle) {
  std::lock_guard<std::mutex> lock(udp_socket_lock_);
  auto socket = udp_socket_.find(handle);
  if (socket == udp_socket_.end()) {
    return nullptr;
  }
  return socket->second;
}

bool ResManager::AsyncMoreTcpAccept(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, int count) {
  for (auto i = 0; i < count; ++i) {
    auto accept_buffer = tcp_buff_pool_.GetAcceptBuffer();
    if (accept_buffer == nullptr) {
      LOG(kError, "tcp handle: %u can not get one accept buffer.", handle);
      return false;
    }
    if (!AsyncTcpAccept(handle, socket, accept_buffer)) {
      return false;
    }
  }
  return true;
}

bool ResManager::AsyncTcpAccept(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, TcpAcceptBuff* buffer) {
  auto accept_socket = new TcpSocket;
  if (!accept_socket->Create(socket->callback())) {
    delete accept_socket;
    tcp_buff_pool_.ReturnAcceptBuff(buffer);
    return false;
  }
  buffer->set_handle(handle);
  buffer->set_accept_socket(accept_socket);
  if (!socket->AsyncAccept(accept_socket->socket(), buffer->buffer(), buffer->ovlp())) {
    delete accept_socket;
    tcp_buff_pool_.ReturnAcceptBuff(buffer);
    return false;
  }
  return true;
}

bool ResManager::AsyncTcpRecv(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, TcpRecvBuff* buffer) {
  buffer->set_handle(handle);
  if (!socket->AsyncRecv(buffer->buffer(), buffer->buffer_size(), buffer->ovlp())) {
    tcp_buff_pool_.ReturnRecvBuff(buffer);
    return false;
  }
  return true;
}

bool ResManager::AsyncMoreUdpRecv(UdpHandle handle, const std::shared_ptr<UdpSocket>& socket, int count) {
  for (auto i = 0; i < count; ++i) {
    auto recv_buffer = udp_buff_pool_.GetRecvBuffer();
    if (recv_buffer == nullptr) {
      LOG(kError, "udp handle: %u can not get one recv buffer.", handle);
      return false;
    }
    if (!AsyncUdpRecv(handle, socket, recv_buffer)) {
      return false;
    }
  }
  return true;
}

bool ResManager::AsyncUdpRecv(UdpHandle handle, const std::shared_ptr<UdpSocket>& socket, UdpRecvBuff* buffer) {
  buffer->set_handle(handle);
  if (!socket->AsyncRecvFrom(buffer->buffer(), buffer->buffer_size(), buffer->ovlp(), buffer->from_addr(), buffer->addr_size())) {
    udp_buff_pool_.ReturnRecvBuff(buffer);
    return false;
  }
  return true;
}

bool ResManager::TransferAsyncType(LPOVERLAPPED ovlp, DWORD transfer_size) {
  auto async_buffer = (BaseBuffer*)ovlp;
  switch (async_buffer->async_type()) {
  case kAsyncTypeTcpAccept:
    return OnTcpAccept((TcpAcceptBuff*)async_buffer);
  case kAsyncTypeTcpSend:
    return OnTcpSend((TcpSendBuff*)async_buffer);
  case kAsyncTypeTcpRecv:
    return OnTcpRecv((TcpRecvBuff*)async_buffer, transfer_size);
  case kAsyncTypeUdpSend:
    return OnUdpSend((UdpSendBuff*)async_buffer);
  case kAsyncTypeUdpRecv:
    return OnUdpRecv((UdpRecvBuff*)async_buffer, transfer_size);
  default:
    return false;
  }
}

bool ResManager::OnTcpAccept(TcpAcceptBuff* buffer) {
  std::shared_ptr<TcpSocket> accept_socket((TcpSocket*)(buffer->accept_socket()));
  auto listen_handle = buffer->handle();
  auto listen_socket = GetTcpSocket(listen_handle);
  if (!listen_socket) {
    tcp_buff_pool_.ReturnAcceptBuff(buffer);
    return true;
  }
  OnTcpAcceptNew(listen_handle, listen_socket, accept_socket);
  buffer->ResetBuffer();
  if (!AsyncTcpAccept(listen_handle, listen_socket, buffer)) {
    OnTcpError(listen_handle, listen_socket->callback(), 1);
    return false;
  }
  return true;
}

bool ResManager::OnTcpSend(TcpSendBuff* buffer) {
  tcp_buff_pool_.ReturnSendBuff(buffer);
  return true;
}

bool ResManager::OnTcpRecv(TcpRecvBuff* buffer, int recv_size) {
  auto recv_handle = buffer->handle();
  auto recv_socket = GetTcpSocket(recv_handle);
  if (!recv_socket) {
    tcp_buff_pool_.ReturnRecvBuff(buffer);
    return true;
  }
  auto callback = recv_socket->callback();
  if (recv_size == 0) {
    tcp_buff_pool_.ReturnRecvBuff(buffer);
    callback->OnTcpDisconnected(recv_handle);
    RemoveTcpSocket(recv_handle);
    return true;
  }
  auto recv_buff = buffer->buffer();
  if (!recv_socket->OnRecv(recv_buff, recv_size)) {
    tcp_buff_pool_.ReturnRecvBuff(buffer);
    OnTcpError(recv_handle, callback, 3);
    return false;
  }
  const auto& all_packet = recv_socket->all_packets();
  for (const auto& i : all_packet) {
    callback->OnTcpReceived(recv_handle, &i[0], i.size());
  }
  recv_socket->OnRecvDone();
  if (!AsyncTcpRecv(recv_handle, recv_socket, buffer)) {
    OnTcpError(recv_handle, callback, 4);
    return false;
  }
  return true;
}

bool ResManager::OnUdpSend(UdpSendBuff* buffer) {
  udp_buff_pool_.ReturnSendBuff(buffer);
  return true;
}

bool ResManager::OnUdpRecv(UdpRecvBuff* buffer, int recv_size) {
  auto recv_handle = buffer->handle();
  auto recv_socket = GetUdpSocket(recv_handle);
  if (!recv_socket) {
    udp_buff_pool_.ReturnRecvBuff(buffer);
    return true;
  }
  std::string ip;
  int port = 0;
  utility::FromSockAddr(*buffer->from_addr(), ip, port);
  auto callback = recv_socket->callback();
  callback->OnUdpReceived(recv_handle, buffer->buffer(), recv_size, ip.c_str(), port);
  if (!AsyncUdpRecv(recv_handle, recv_socket, buffer)) {
    OnUdpError(recv_handle, callback, 1);
    return false;
  }
  return true;
}

bool ResManager::OnTcpAcceptNew(TcpHandle listen_handle, const std::shared_ptr<TcpSocket>& listen_socket, const std::shared_ptr<TcpSocket>& accept_socket) {
  auto accept_handle = kInvalidTcpHandle;
  if (!NewTcpSocket(accept_handle, accept_socket)) {
    return false;
  }
  if (!accept_socket->SetAccepted(listen_socket->socket())) {
    RemoveTcpSocket(accept_handle);
    return false;
  }
  if (!iocp_.BindToIOCP(accept_socket->socket())) {
    RemoveTcpSocket(accept_handle);
    return false;
  }
  auto callback = accept_socket->callback();
  callback->OnTcpAccepted(listen_handle, accept_handle);
  auto recv_buffer = tcp_buff_pool_.GetRecvBuffer();
  if (recv_buffer == nullptr) {
    OnTcpError(accept_handle, callback, 2);
    return false;
  }
  if (!AsyncTcpRecv(accept_handle, accept_socket, recv_buffer)) {
    OnTcpError(accept_handle, callback, 4);
    return false;
  }
  return true;
}

void ResManager::OnTcpError(TcpHandle handle, NetInterface* callback, int error) {
  LOG(kError, "tcp handle %u error: %d.", handle, error);
  if (callback != nullptr) {
    callback->OnTcpError(handle, error);
  }
  RemoveTcpSocket(handle);
}

void ResManager::OnUdpError(UdpHandle handle, NetInterface* callback, int error) {
  LOG(kError, "udp handle %u error: %d.", handle, error);
  if (callback != nullptr) {
    callback->OnUdpError(handle, error);
  }
  RemoveUdpSocket(handle);
}

} // namespace net