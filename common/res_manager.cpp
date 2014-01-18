#include "res_manager.h"
#include "log.h"
#include "utility.h"
#include "utility_net.h"

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
  net_started_ = false;
  return true;
}

bool ResManager::TcpCreate(NetInterface* callback, const std::string& ip, int port, TcpHandle& new_handle) {
  if (callback == nullptr) {
    LOG(kError, "create tcp handle failed: invalid callback parameter.");
    return false;
  }
  std::shared_ptr<TcpSocket> new_socket(new TcpSocket);
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
    return false;
  }
  auto backlog = utility::GetProcessorNum() * 2;
  if (!socket->Listen(backlog)) {
    return false;
  }
  for (auto i = 0; i < backlog; ++i) {
    auto accept_buffer = GetTcpAcceptBuffer();
    if (accept_buffer == nullptr) {
      return false;
    }
    if (!AsyncTcpAccept(handle, socket, accept_buffer)) {
      return false;
    }
  }
  return true;
}

bool ResManager::TcpConnect(TcpHandle handle, const std::string& ip, int port) {
  auto socket = GetTcpSocket(handle);
  if (!socket) {
    return false;
  }
  if (!socket->Connect(ip, port)) {
    return false;
  }
  auto recv_buffer = GetTcpRecvBuffer();
  if (recv_buffer == nullptr) {
    return false;
  }
  if (!AsyncTcpRecv(handle, socket, recv_buffer)) {
    return false;
  }
  return true;
}

bool ResManager::TcpSend(TcpHandle handle, std::unique_ptr<char[]> packet, int size) {
  if (!packet || size <= 0 || size > kMaxTcpPacketSize) {
    LOG(kError, "send tcp handle: %u packet failed: invalid parameter.", handle);
    return false;
  }
  auto socket = GetTcpSocket(handle);
  if (!socket) {
    return false;
  }
  auto send_buffer = GetTcpSendBuffer();
  if (send_buffer == nullptr) {
    return false;
  }
  auto deleter = packet.get_deleter();
  auto packet_ptr = packet.release();
  if (!send_buffer->Init(packet_ptr, size, deleter)) {
    ReturnTcpSendBuffer(send_buffer);
    return false;
  }
  send_buffer->set_handle(handle);
  if (!socket->AsyncSend(send_buffer->header(), send_buffer->buffer(), send_buffer->buffer_size(), send_buffer->ovlp())) {
    ReturnTcpSendBuffer(send_buffer);
    return false;
  }
  return true;
}

bool ResManager::TcpGetLocalAddr(TcpHandle handle, char ip[16], int& port) {
  if (ip == nullptr) {
    LOG(kError, "get tcp handle : %u local address failed: invalid ip parameter.", handle);
    return false;
  }
  auto socket = GetTcpSocket(handle);
  if (!socket) {
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
    return false;
  }
  std::string ip_string;
  if (!socket->GetRemoteAddr(ip_string, port)) {
    return false;
  }
  strcpy_s(ip, 16, ip_string.c_str());
  return true;
}

bool ResManager::UdpCreate(NetInterface* callback, const std::string& ip, int port, UdpHandle& new_handle) {
  if (callback == nullptr) {
    LOG(kError, "create udp handle failed: invalid callback parameter.");
    return false;
  }
  std::shared_ptr<UdpSocket> new_socket(new UdpSocket);
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
  for (auto i = 0; i < recv_count; ++i) {
    auto recv_buffer = GetUdpRecvBuffer();
    if (recv_buffer == nullptr) {
      return false;
    }
    if (!AsyncUdpRecv(new_handle, new_socket, recv_buffer)) {
      return false;
    }
  }
  return true;
}

bool ResManager::UdpDestroy(UdpHandle handle) {
  RemoveUdpSocket(handle);
  return true;
}

bool ResManager::UdpSendTo(UdpHandle handle, std::unique_ptr<char[]> packet, int size, const std::string& ip, int port) {
  if (!packet || size <= 0 || size > kMaxUdpPacketSize || port <= 0) {
    LOG(kError, "send udp handle: %u packet failed: invalid parameter.", handle);
    return false;
  }
  auto socket = GetUdpSocket(handle);
  if (!socket) {
    return false;
  }
  auto send_buffer = GetUdpSendBuffer();
  if (send_buffer == nullptr) {
    return false;
  }
  auto deleter = packet.get_deleter();
  auto packet_ptr = packet.release();
  if (!send_buffer->Init(packet_ptr, size, deleter)) {
    ReturnUdpSendBuffer(send_buffer);
    return false;
  }
  send_buffer->set_handle(handle);
  if (!socket->AsyncSendTo(send_buffer->buffer(), send_buffer->buffer_size(), ip, port, send_buffer->ovlp())) {
    ReturnUdpSendBuffer(send_buffer);
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
    LOG(kError, "can not find tcp handle: %u.", handle);
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

TcpAcceptBuffer* ResManager::GetTcpAcceptBuffer() {
  auto buffer = new TcpAcceptBuffer;
  return buffer;
}

TcpSendBuffer* ResManager::GetTcpSendBuffer() {
  auto buffer = new TcpSendBuffer;
  return buffer;
}

TcpRecvBuffer* ResManager::GetTcpRecvBuffer() {
  auto buffer = new TcpRecvBuffer;
  return buffer;
}

UdpSendBuffer* ResManager::GetUdpSendBuffer() {
  auto buffer = new UdpSendBuffer;
  return buffer;
}

UdpRecvBuffer* ResManager::GetUdpRecvBuffer() {
  auto buffer = new UdpRecvBuffer;
  return buffer;
}

void ResManager::ReturnTcpAcceptBuffer(TcpAcceptBuffer* buffer) {
  if (buffer != nullptr) {
    delete buffer;
  }
}

void ResManager::ReturnTcpSendBuffer(TcpSendBuffer* buffer) {
  if (buffer != nullptr) {
    delete buffer;
  }
}

void ResManager::ReturnTcpRecvBuffer(TcpRecvBuffer* buffer) {
  if (buffer != nullptr) {
    delete buffer;
  }
}

void ResManager::ReturnUdpSendBuffer(UdpSendBuffer* buffer) {
  if (buffer != nullptr) {
    delete buffer;
  }
}

void ResManager::ReturnUdpRecvBuffer(UdpRecvBuffer* buffer) {
  if (buffer != nullptr) {
    delete buffer;
  }
}

bool ResManager::AsyncTcpAccept(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, TcpAcceptBuffer* buffer) {
  auto accept_socket = new TcpSocket;
  if (!accept_socket->Create(socket->callback())) {
    delete accept_socket;
    ReturnTcpAcceptBuffer(buffer);
    return false;
  }
  buffer->set_handle(handle);
  buffer->set_accept_socket(accept_socket);
  if (!socket->AsyncAccept(accept_socket->socket(), buffer->buffer(), buffer->buffer_size(), buffer->ovlp())) {
    delete accept_socket;
    ReturnTcpAcceptBuffer(buffer);
    return false;
  }
  return true;
}

bool ResManager::AsyncTcpRecv(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, TcpRecvBuffer* buffer) {
  buffer->set_handle(handle);
  if (!socket->AsyncRecv(buffer->buffer(), buffer->buffer_size(), buffer->ovlp())) {
    ReturnTcpRecvBuffer(buffer);
    return false;
  }
  return true;
}

bool ResManager::AsyncUdpRecv(UdpHandle handle, const std::shared_ptr<UdpSocket>& socket, UdpRecvBuffer* buffer) {
  buffer->set_handle(handle);
  if (!socket->AsyncRecvFrom(buffer->buffer(), buffer->buffer_size(), buffer->ovlp(), buffer->from_addr(), buffer->addr_size())) {
    ReturnUdpRecvBuffer(buffer);
    return false;
  }
  return true;
}

bool ResManager::TransferAsyncType(LPOVERLAPPED ovlp, DWORD transfer_size) {
  auto async_buffer = (BaseBuffer*)ovlp;
  switch (async_buffer->async_type()) {
  case kAsyncTypeTcpAccept:
    return OnTcpAccept((TcpAcceptBuffer*)async_buffer);
  case kAsyncTypeTcpSend:
    return OnTcpSend((TcpSendBuffer*)async_buffer);
  case kAsyncTypeTcpRecv:
    return OnTcpRecv((TcpRecvBuffer*)async_buffer, transfer_size);
  case kAsyncTypeUdpSend:
    return OnUdpSend((UdpSendBuffer*)async_buffer);
  case kAsyncTypeUdpRecv:
    return OnUdpRecv((UdpRecvBuffer*)async_buffer, transfer_size);
  default:
    return false;
  }
}

bool ResManager::OnTcpAccept(TcpAcceptBuffer* buffer) {
  std::shared_ptr<TcpSocket> accept_socket((TcpSocket*)(buffer->accept_socket()));
  auto listen_handle = buffer->handle();
  auto listen_socket = GetTcpSocket(listen_handle);
  if (!listen_socket) {
    ReturnTcpAcceptBuffer(buffer);
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

bool ResManager::OnTcpSend(TcpSendBuffer* buffer) {
  ReturnTcpSendBuffer(buffer);
  return true;
}

bool ResManager::OnTcpRecv(TcpRecvBuffer* buffer, int size) {
  auto recv_handle = buffer->handle();
  auto recv_socket = GetTcpSocket(recv_handle);
  if (!recv_socket) {
    ReturnTcpRecvBuffer(buffer);
    return true;
  }
  auto callback = recv_socket->callback();
  if (size == 0) {
    ReturnTcpRecvBuffer(buffer);
    callback->OnTcpDisconnected(recv_handle);
    RemoveTcpSocket(recv_handle);
    return true;
  }
  auto recv_buff = buffer->buffer();
  if (!recv_socket->OnRecv(recv_buff, size)) {
    ReturnTcpRecvBuffer(buffer);
    OnTcpError(recv_handle, callback, 3);
    return false;
  }
  const auto& all_packet = recv_socket->all_packets();
  for (const auto& i : all_packet) {
    callback->OnTcpReceived(recv_handle, i.packet, i.size);
  }
  recv_socket->OnRecvDone();
  if (!AsyncTcpRecv(recv_handle, recv_socket, buffer)) {
    OnTcpError(recv_handle, callback, 4);
    return false;
  }
  return true;
}

bool ResManager::OnUdpSend(UdpSendBuffer* buffer) {
  ReturnUdpSendBuffer(buffer);
  return true;
}

bool ResManager::OnUdpRecv(UdpRecvBuffer* buffer, int size) {
  auto recv_handle = buffer->handle();
  auto recv_socket = GetUdpSocket(recv_handle);
  if (!recv_socket) {
    ReturnUdpRecvBuffer(buffer);
    return true;
  }
  std::string ip;
  int port = 0;
  utility::FromSockAddr(*buffer->from_addr(), ip, port);
  auto callback = recv_socket->callback();
  callback->OnUdpReceived(recv_handle, buffer->buffer(), size, ip, port);
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
  auto recv_buffer = GetTcpRecvBuffer();
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