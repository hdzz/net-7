#include "tcp_buff_pool.h"

namespace net {

TcpBuffPool::TcpBuffPool()
{
}

TcpBuffPool::~TcpBuffPool()
{
}

bool TcpBuffPool::Init() {
  for (auto i=0; i<kTCPSendBuffPoolSize; ++i) {
    auto send_buffer = new TcpSendBuff;
    send_buffer_.push_back(send_buffer);
  }
  for (auto i=0; i<kTCPRecvBuffPoolSize; ++i) {
    auto recv_buffer = new TcpRecvBuff;
    recv_buffer_.push_back(recv_buffer);
  }
  for (auto i=0; i<kAcceptBuffPoolSize; ++i) {
    auto accept_buffer = new AcceptBuff;
    accept_buffer_.push_back(accept_buffer);
  }
  return true;
}

void TcpBuffPool::Clear() {
  do {
    std::lock_guard<std::mutex> lock(send_buff_lock_);
    for (auto i : send_buffer_) { delete i; }
    send_buffer_.clear();
  } while (false);
  do {
    std::lock_guard<std::mutex> lock(recv_buff_lock_);
    for (auto i : recv_buffer_) { delete i; }
    recv_buffer_.clear();
  } while (false);
  do {
    std::lock_guard<std::mutex> lock(accept_buff_lock_);
    for (auto i : accept_buffer_) { delete i; }
    accept_buffer_.clear();
  } while (false);
}

const int kTrueFirstBuffSize = kTcpBufferSize - kTcpHeadSize; // TCP首个缓冲实际可用大小

int TcpBuffPool::CalcSendBuffNum(int packet_size) {
  if (packet_size <= kTrueFirstBuffSize) {
    return 1;
  }
  auto buffer_num = 1;
  auto left_size = packet_size - kTrueFirstBuffSize;
  buffer_num += left_size / kTcpBufferSize;
  if (left_size % kTcpBufferSize > 0) {
    ++buffer_num;
  }
  return buffer_num;
}

// 填充所有发送缓冲
void TcpBuffPool::FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer) {
  auto fill_offset = packet;
  auto fill_left = size;
  int buffer_num = buffer.size();
  for (auto i=0; i<buffer_num; ++i) {
    auto& each_buffer = buffer.at(i);
    if (i == 0) {
      TcpHead head;
      head.packet_size_ = size;
      head.checksum_ = 0;
      each_buffer->set_buffer( 0,(char*)&head, kTcpHeadSize);
      if (fill_left >= kTrueFirstBuffSize) {
        each_buffer->set_buffer(kTcpHeadSize, fill_offset, kTrueFirstBuffSize);
        fill_offset += kTrueFirstBuffSize;
        fill_left -= kTrueFirstBuffSize;
        each_buffer->set_buffer_size(kTcpBufferSize);
      } else {
        each_buffer->set_buffer(kTcpHeadSize, fill_offset, fill_left);
        each_buffer->set_buffer_size(kTcpHeadSize + fill_left);
        break;
      }
    } else {
      if (fill_left >= kTcpBufferSize) {
        each_buffer->set_buffer(0, fill_offset, kTcpBufferSize);
        fill_offset += kTcpBufferSize;
        fill_left -= kTcpBufferSize;
        each_buffer->set_buffer_size(kTcpBufferSize);
      } else {
        each_buffer->set_buffer(0, fill_offset, fill_left);
        each_buffer->set_buffer_size(fill_left);
        break;
      }
    }
  }
}

AcceptBuff* TcpBuffPool::GetAcceptBuffer() {
  std::lock_guard<std::mutex> lock(accept_buff_lock_);
  if (accept_buffer_.empty()){
    return nullptr;
  }
  auto ret_buffer = *(accept_buffer_.begin());
  accept_buffer_.pop_front();
  return ret_buffer;
}

std::vector<TcpSendBuff*> TcpBuffPool::GetSendBuffer(const char* buffer, int size) {
  std::vector<TcpSendBuff*> all_buffer;
  auto is_buff_enough = false;
  size_t buffer_num = CalcSendBuffNum(size);
  do {
    std::lock_guard<std::mutex> lock(send_buff_lock_);
    if (send_buffer_.size() >= buffer_num){
      is_buff_enough = true;
      for (size_t i=0; i<buffer_num; ++i) {
        auto one_buffer = *(send_buffer_.begin());
        send_buffer_.pop_front();
        all_buffer.push_back(one_buffer);
      }
    }
  } while (false);
  if (is_buff_enough) {
    FillTcpSendBuff(buffer, size, all_buffer);
  }
  return std::move(all_buffer);
}

TcpRecvBuff* TcpBuffPool::GetRecvBuffer() {
  std::lock_guard<std::mutex> lock(recv_buff_lock_);
  if (recv_buffer_.empty()){
    return nullptr;
  }
  auto ret_buffer = *(recv_buffer_.begin());
  recv_buffer_.pop_front();
  return ret_buffer;
}

void TcpBuffPool::ReturnAcceptBuff(AcceptBuff* buffer) {
  buffer->ResetBuffer();
  std::lock_guard<std::mutex> lock(accept_buff_lock_);
  accept_buffer_.push_back(buffer);
}

void TcpBuffPool::ReturnTcpSendBuff(TcpSendBuff* buffer) {
  buffer->ResetBuffer();
  std::lock_guard<std::mutex> lock(send_buff_lock_);
  send_buffer_.push_back(buffer);
}

void TcpBuffPool::ReturnTcpRecvBuff(TcpRecvBuff* buffer) {
  buffer->ResetBuffer();
  std::lock_guard<std::mutex> lock(recv_buff_lock_);
  recv_buffer_.push_back(buffer);
}

} // namespace net