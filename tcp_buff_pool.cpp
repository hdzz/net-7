#include "tcp_buff_pool.h"

namespace net {

TcpBuffPool::TcpBuffPool()
{
}

TcpBuffPool::~TcpBuffPool()
{
}

bool TcpBuffPool::Init() {
  for (int i=0; i<kTCPSendBuffPoolSize; ++i) {
    auto send_buffer = new TcpSendBuff;
    send_buff_.push_back(send_buffer);
  }
  for (int i=0; i<kTCPRecvBuffPoolSize; ++i) {
    auto recv_buffer = new TcpRecvBuff;
    recv_buff_.push_back(recv_buffer);
  }
  for (int i=0; i<kAcceptBuffPoolSize; ++i) {
    auto accept_buffer = new AcceptBuff;
    accept_buff_.push_back(accept_buffer);
  }
  return true;
}

void TcpBuffPool::Clear() {
  do {
    std::lock_guard<std::mutex> lock(send_buff_lock_);
    for (auto i : send_buff_) { delete i; }
    send_buff_.clear();
  } while (false);
  do {
    std::lock_guard<std::mutex> lock(recv_buff_lock_);
    for (auto i : recv_buff_) { delete i; }
    recv_buff_.clear();
  } while (false);
  do {
    std::lock_guard<std::mutex> lock(accept_buff_lock_);
    for (auto i : accept_buff_) { delete i; }
    accept_buff_.clear();
  } while (false);
}

AcceptBuff* TcpBuffPool::GetAcceptBuffer() {
  std::lock_guard<std::mutex> lock(accept_buff_lock_);
  if (accept_buff_.empty()){
    return nullptr;
  }
  auto ret_buffer = *(accept_buff_.begin());
  accept_buff_.pop_front();
  return ret_buffer;
}

TcpSendBuff* TcpBuffPool::GetSendBuffer() {
  std::lock_guard<std::mutex> lock(send_buff_lock_);
  if (send_buff_.empty()){
    return nullptr;
  }
  auto ret_buffer = *(send_buff_.begin());
  send_buff_.pop_front();
  return ret_buffer;
}

TcpRecvBuff* TcpBuffPool::GetRecvBuffer() {
  std::lock_guard<std::mutex> lock(recv_buff_lock_);
  if (recv_buff_.empty()){
    return nullptr;
  }
  auto ret_buffer = *(recv_buff_.begin());
  recv_buff_.pop_front();
  return ret_buffer;
}

void TcpBuffPool::ReturnAcceptBuff(AcceptBuff* buffer) {
  buffer->ResetBuffer();
  std::lock_guard<std::mutex> lock(accept_buff_lock_);
  accept_buff_.push_back(buffer);
}

void TcpBuffPool::ReturnTcpSendBuff(TcpSendBuff* buffer) {
  buffer->ResetBuffer();
  std::lock_guard<std::mutex> lock(send_buff_lock_);
  send_buff_.push_back(buffer);
}

void TcpBuffPool::ReturnTcpRecvBuff(TcpRecvBuff* buffer) {
  buffer->ResetBuffer();
  std::lock_guard<std::mutex> lock(recv_buff_lock_);
  recv_buff_.push_back(buffer);
}

} // namespace net