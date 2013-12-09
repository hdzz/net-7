#include "tcp_buff_pool.h"
#include "tcp_header.h"
#include "log.h"

namespace net {

const int kTCPSendBuffPoolSize = 1024;
const int kTCPRecvBuffPoolSize = 1024;
const int kAcceptBuffPoolSize = 64;

TcpBuffPool::TcpBuffPool() {
}

TcpBuffPool::~TcpBuffPool() {
}

void TcpBuffPool::Init() {
	ExpandAcceptBuff();
	ExpandSendBuff();
	ExpandRecvBuff();
}

void TcpBuffPool::Clear() {
	do {
		std::lock_guard<std::mutex> lock(accept_buff_lock_);
		for (const auto& i : accept_buffer_) { delete i; }
		accept_buffer_.clear();
	} while (false);
  do {
		std::lock_guard<std::mutex> lock(send_buff_lock_);
    for (const auto& i : send_buffer_) { delete i; }
    send_buffer_.clear();
  } while (false);
  do {
		std::lock_guard<std::mutex> lock(recv_buff_lock_);
		for (const auto& i : recv_buffer_) { delete i; }
    recv_buffer_.clear();
  } while (false);
}

AcceptBuff* TcpBuffPool::GetAcceptBuffer() {
	std::lock_guard<std::mutex> lock(accept_buff_lock_);
	if (accept_buffer_.empty()) { ExpandAcceptBuff(); }
	auto accept_buffer = accept_buffer_.front();
	accept_buffer_.pop_front();
	return accept_buffer;
}

std::vector<TcpSendBuff*> TcpBuffPool::GetSendBuffer(const char* buffer, int size) {
	std::vector<TcpSendBuff*> all_buffer;
	size_t buffer_num = CalcSendBuffNum(size);
	do {
		std::lock_guard<std::mutex> lock(send_buff_lock_);
		while (send_buffer_.size() < buffer_num) { ExpandSendBuff(); }
		for (size_t i = 0; i < buffer_num; ++i) {
			auto send_buffer = send_buffer_.front();
			send_buffer_.pop_front();
			all_buffer.push_back(send_buffer);
		}
	} while (false);
	FillTcpSendBuff(buffer, size, all_buffer);
	return std::move(all_buffer);
}

TcpRecvBuff* TcpBuffPool::GetRecvBuffer() {
	std::lock_guard<std::mutex> lock(recv_buff_lock_);
	if (recv_buffer_.empty()) { ExpandRecvBuff(); }
	auto recv_buffer = recv_buffer_.front();
	recv_buffer_.pop_front();
	return recv_buffer;
}

void TcpBuffPool::ReturnAcceptBuff(AcceptBuff* buffer) {
	buffer->ResetBuffer();
	std::lock_guard<std::mutex> lock(accept_buff_lock_);
	accept_buffer_.push_back(buffer);
	TryNarrowAcceptBuff();
}

void TcpBuffPool::ReturnSendBuff(TcpSendBuff* buffer) {
	buffer->ResetBuffer();
	std::lock_guard<std::mutex> lock(send_buff_lock_);
	send_buffer_.push_back(buffer);
	TryNarrowSendBuff();
}

void TcpBuffPool::ReturnRecvBuff(TcpRecvBuff* buffer) {
	buffer->ResetBuffer();
	std::lock_guard<std::mutex> lock(recv_buff_lock_);
	recv_buffer_.push_back(buffer);
	TryNarrowRecvBuff();
}

void TcpBuffPool::ExpandAcceptBuff() {
	for (auto i = 0; i < kAcceptBuffPoolSize; ++i) {
		auto accept_buffer = new AcceptBuff;
		accept_buffer_.push_back(accept_buffer);
	}
}

void TcpBuffPool::ExpandSendBuff() {
	for (auto i = 0; i < kTCPSendBuffPoolSize; ++i) {
		auto send_buffer = new TcpSendBuff;
		send_buffer_.push_back(send_buffer);
	}
}

void TcpBuffPool::ExpandRecvBuff() {
	for (auto i = 0; i < kTCPRecvBuffPoolSize; ++i) {
		auto recv_buffer = new TcpRecvBuff;
		recv_buffer_.push_back(recv_buffer);
	}
}

void TcpBuffPool::TryNarrowAcceptBuff() {
	int narrow_size = 0;
	auto now_size = accept_buffer_.size();
	if (now_size >= 2 * kAcceptBuffPoolSize) {
		narrow_size = now_size / 2;
	} else if (now_size > kAcceptBuffPoolSize) {
		narrow_size = kAcceptBuffPoolSize;
	}
	for (auto i = 0; i < narrow_size; ++i) {
		delete accept_buffer_.front();
		accept_buffer_.pop_front();
	}
}

void TcpBuffPool::TryNarrowSendBuff() {
	int narrow_size = 0;
	auto now_size = send_buffer_.size();
	if (now_size >= 2 * kTCPSendBuffPoolSize) {
		narrow_size = now_size / 2;
	}
	else if (now_size > kTCPSendBuffPoolSize) {
		narrow_size = kTCPSendBuffPoolSize;
	}
	for (auto i = 0; i < narrow_size; ++i) {
		delete send_buffer_.front();
		send_buffer_.pop_front();
	}
}

void TcpBuffPool::TryNarrowRecvBuff() {
	int narrow_size = 0;
	auto now_size = recv_buffer_.size();
	if (now_size >= 2 * kTCPRecvBuffPoolSize) {
		narrow_size = now_size / 2;
	}
	else if (now_size > kTCPRecvBuffPoolSize) {
		narrow_size = kTCPRecvBuffPoolSize;
	}
	for (auto i = 0; i < narrow_size; ++i) {
		delete recv_buffer_.front();
		recv_buffer_.pop_front();
	}
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

void TcpBuffPool::FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer) {
  auto fill_offset = packet;
  auto fill_left = size;
  int buffer_num = buffer.size();
  for (auto i=0; i<buffer_num; ++i) {
    auto& each_buffer = buffer.at(i);
    if (i == 0) {
      TcpHeader header;
			header.Init(size);
			each_buffer->set_buffer(0, header, kTcpHeadSize);
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

} // namespace net