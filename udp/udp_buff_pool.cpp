#include "udp_buff_pool.h"
#include "log.h"

namespace net {

const int kUDPSendBuffPoolSize = 1024;
const int kUDPRecvBuffPoolSize = 1024;

UdpBuffPool::UdpBuffPool() {
}

UdpBuffPool::~UdpBuffPool() {
}

void UdpBuffPool::Init() {
	ExpandSendBuff();
	ExpandRecvBuff();
}

void UdpBuffPool::Clear() {
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

UdpSendBuff* UdpBuffPool::GetSendBuffer() {
	std::lock_guard<std::mutex> lock(send_buff_lock_);
	if (send_buffer_.empty()) { ExpandSendBuff(); }
	auto send_buffer = send_buffer_.front();
	send_buffer_.pop_front();
	return std::move(send_buffer);
}

UdpRecvBuff* UdpBuffPool::GetRecvBuffer() {
	std::lock_guard<std::mutex> lock(recv_buff_lock_);
	if (recv_buffer_.empty()) { ExpandRecvBuff(); }
	auto recv_buffer = recv_buffer_.front();
	recv_buffer_.pop_front();
	return recv_buffer;
}

void UdpBuffPool::ReturnSendBuff(UdpSendBuff* buffer) {
	buffer->ResetBuffer();
	std::lock_guard<std::mutex> lock(send_buff_lock_);
	send_buffer_.push_back(buffer);
	TryNarrowSendBuff();
}

void UdpBuffPool::ReturnRecvBuff(UdpRecvBuff* buffer) {
	buffer->ResetBuffer();
	std::lock_guard<std::mutex> lock(recv_buff_lock_);
	recv_buffer_.push_back(buffer);
	TryNarrowRecvBuff();
}

void UdpBuffPool::ExpandSendBuff() {
	for (auto i = 0; i < kUDPSendBuffPoolSize; ++i) {
		auto send_buffer = new UdpSendBuff;
		send_buffer_.push_back(send_buffer);
	}
}

void UdpBuffPool::ExpandRecvBuff() {
	for (auto i = 0; i < kUDPRecvBuffPoolSize; ++i) {
		auto recv_buffer = new UdpRecvBuff;
		recv_buffer_.push_back(recv_buffer);
	}
}

void UdpBuffPool::TryNarrowSendBuff() {
	int narrow_size = 0;
	auto now_size = send_buffer_.size();
	if (now_size >= 2 * kUDPSendBuffPoolSize) {
		narrow_size = now_size / 2;
	}
	else if (now_size > kUDPSendBuffPoolSize) {
		narrow_size = kUDPSendBuffPoolSize;
	}
	for (auto i = 0; i < narrow_size; ++i) {
		delete send_buffer_.front();
		send_buffer_.pop_front();
	}
}

void UdpBuffPool::TryNarrowRecvBuff() {
	int narrow_size = 0;
	auto now_size = recv_buffer_.size();
	if (now_size >= 2 * kUDPRecvBuffPoolSize) {
		narrow_size = now_size / 2;
	}
	else if (now_size > kUDPRecvBuffPoolSize) {
		narrow_size = kUDPRecvBuffPoolSize;
	}
	for (auto i = 0; i < narrow_size; ++i) {
		delete recv_buffer_.front();
		recv_buffer_.pop_front();
	}
}

} // namespace net