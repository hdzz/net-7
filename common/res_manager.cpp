#include "res_manager.h"
#include "tcp_header.h"
#include "log.h"

namespace net {

const int kAsyncUDPRecvNum = 64;

ResManager::ResManager() {
  is_net_started_ = false;
  tcp_link_count_ = 0;
	udp_link_count_ = 0;
}

ResManager::~ResManager() {
  CleanupNet();
}

bool ResManager::StartupNet() {
	if (is_net_started_) {
		return true;
	}
  is_net_started_ = true;
	tcp_buff_pool_.Init();
	udp_buff_pool_.Init();
	auto iocp_callback = std::bind(&ResManager::TransferAsyncType, this, std::placeholders::_1, std::placeholders::_2);
	if (!iocp_.Init(iocp_callback)) {
		CleanupNet();
		return false;
	}
	LOG(kStartup, "net started.");
	return true;
}

bool ResManager::CleanupNet() {
	if (!is_net_started_) {
		return true;
	}
	iocp_.Uninit();
  do {
    std::lock_guard<std::mutex> lock(tcp_link_lock_);
    for (const auto& i : tcp_link_) { delete i.second; }
    tcp_link_.clear();
  } while (false);
	tcp_link_count_ = 0;
  tcp_buff_pool_.Clear();
	do {
		std::lock_guard<std::mutex> lock(udp_link_lock_);
		for (const auto& i : udp_link_) { delete i.second; }
		udp_link_.clear();
	} while (false);
	udp_link_count_ = 0;
	udp_buff_pool_.Clear();
  is_net_started_ = false;
	LOG(kStartup, "net cleaned.");
	return true;
}

bool ResManager::CreateTcpLink(NetInterface* callback, const char* ip, int port, TcpLink& new_link) {
	if (callback == nullptr) {
		return false;
	}
	if (ip == nullptr) {
		ip = "0.0.0.0";
	}
	auto new_socket = new TcpSocket;
	if (!new_socket->Create(callback)) {
		delete new_socket;
		return false;
	}
	if (!new_socket->Bind(ip, port)) {
		delete new_socket;
		return false;
	}
	if (!iocp_.BindToIOCP(new_socket->socket())) {
		delete new_socket;
		return false;
	}
	new_link = AddTcpLink(new_socket);
	return true;
}

bool ResManager::DestroyTcpLink(TcpLink link) {
	RemoveTcpLink(link);
	return true;
}

bool ResManager::Listen(TcpLink link, int backlog) {
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	if (backlog > kMaxListenBacklogSize) {
		backlog = kMaxListenBacklogSize;
  }
	if (!find_socket->Listen(backlog)) {
		return false;
	}
	if (!AsyncMoreAccept(link, find_socket, backlog)) {
		return false;
	}
	return true;
}

bool ResManager::Connect(TcpLink link, const char* ip, int port) {
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	if (!find_socket->Connect(ip, port)) {
		return false;
	}
	auto recv_buffer = tcp_buff_pool_.GetRecvBuffer();
	if (recv_buffer == nullptr) {
		return false;
	}
	return AsyncTcpRecv(link, find_socket, recv_buffer);
}

bool ResManager::SendTcpPacket(TcpLink link, const char* packet, int size) {
	if (packet == nullptr || size <= 0 || size > kMaxTcpPacketSize) {
		return false;
	}
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	auto send_buffer = tcp_buff_pool_.GetSendBuffer(packet, size);
	for (auto& i : send_buffer) {
		i->set_link(link);
		if (!find_socket->AsyncSend(i->buffer(), i->buffer_size(), (LPOVERLAPPED)i)) {
			tcp_buff_pool_.ReturnSendBuff(i);
			return false;
		}
	}
	return true;
}

bool ResManager::GetTcpLinkLocalAddr(TcpLink link, char ip[16], int& port) {
	if (ip == nullptr) {
		return false;
	}
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
  std::string ip_string;
  find_socket->GetLocalAddr(ip_string, port);
  strcpy_s(ip, 16, ip_string.c_str());
	return true;
}

bool ResManager::GetTcpLinkRemoteAddr(TcpLink link, char ip[16], int& port) {
	if (ip == nullptr) {
		return false;
	}
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	std::string ip_string;
	find_socket->GetRemoteAddr(ip_string, port);
	strcpy_s(ip, 16, ip_string.c_str());
	return true;
}

bool ResManager::CreateUdpLink(NetInterface* callback, const char* ip, int port, bool broadcast, UdpLink& new_link) {
	if (callback == nullptr) {
		return false;
	}
	if (ip == nullptr) {
		ip = "0.0.0.0";
	}
	auto new_socket = new UdpSocket;
	if (!new_socket->Create(callback, broadcast)) {
		delete new_socket;
		return false;
	}
	if (!new_socket->Bind(ip, port)) {
		delete new_socket;
		return false;
	}
	if (!iocp_.BindToIOCP(new_socket->socket())) {
		delete new_socket;
		return false;
	}
	new_link = AddUdpLink(new_socket);
	for (auto i = 0; i < kAsyncUDPRecvNum; ++i) {
		auto recv_buffer = udp_buff_pool_.GetRecvBuffer();
		if (recv_buffer == nullptr) {
			continue;
		}
		AsyncUdpRecv(new_link, new_socket, recv_buffer);
	}
	return true;
}

bool ResManager::DestroyUdpLink(UdpLink link) {
	RemoveUdpLink(link);
	return true;
}

bool ResManager::SendUdpPacket(UdpLink link, const char* packet, int size, const char* ip, int port) {
	if (packet == nullptr || size <= 0 || size > kMaxUdpPacketSize || ip == nullptr || port <= 0) {
		return false;
	}
	auto find_socket = FindUdpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	auto send_buffer = udp_buff_pool_.GetSendBuffer();
	if (send_buffer == nullptr) {
		return false;
	}
	send_buffer->set_buffer(packet, size);
	send_buffer->set_link(link);
	if (!find_socket->AsyncSendTo(send_buffer->buffer(), send_buffer->buffer_size(), ip, port, (LPOVERLAPPED)send_buffer)) {
		udp_buff_pool_.ReturnSendBuff(send_buffer);
		return false;
	}
	return true;
}

TcpLink ResManager::AddTcpLink(TcpSocket* new_socket) {
	std::lock_guard<std::mutex> lock(tcp_link_lock_);
	TcpLink new_link = ++tcp_link_count_;
	tcp_link_.insert(std::make_pair(new_link, new_socket));
	return new_link;
}

void ResManager::RemoveTcpLink(TcpLink link) {
	std::lock_guard<std::mutex> lock(tcp_link_lock_);
	auto find_link = tcp_link_.find(link);
	if (find_link == tcp_link_.end()) {
		return;
	}
	auto find_socket = find_link->second;
	delete find_socket;
	tcp_link_.erase(find_link);
	return;
}

TcpSocket* ResManager::FindTcpSocket(TcpLink link) {
	std::lock_guard<std::mutex> lock(tcp_link_lock_);
	auto find_link = tcp_link_.find(link);
	if (find_link == tcp_link_.end()) {
		return nullptr;
	}
	return find_link->second;
}

UdpLink ResManager::AddUdpLink(UdpSocket* new_socket) {
	std::lock_guard<std::mutex> lock(udp_link_lock_);
	UdpLink new_link = ++udp_link_count_;
	udp_link_.insert(std::make_pair(new_link, new_socket));
	return new_link;
}

void ResManager::RemoveUdpLink(UdpLink link) {
	std::lock_guard<std::mutex> lock(udp_link_lock_);
	auto find_link = udp_link_.find(link);
	if (find_link == udp_link_.end()) {
		return;
	}
	auto find_socket = find_link->second;
	delete find_socket;
	udp_link_.erase(find_link);
	return;
}

UdpSocket* ResManager::FindUdpSocket(UdpLink link) {
	std::lock_guard<std::mutex> lock(udp_link_lock_);
	auto find_link = udp_link_.find(link);
	if (find_link == udp_link_.end()) {
		return nullptr;
	}
	return find_link->second;
}

bool ResManager::AsyncMoreAccept(TcpLink link, TcpSocket* socket, int count) {
	auto is_success = false;
	for (auto i=0; i<count; ++i) {
		auto accept_buffer = tcp_buff_pool_.GetAcceptBuffer();
		if (accept_buffer == nullptr) {
			continue;
		}
		if (!AsyncAccept(link, socket, accept_buffer)) {
			continue;
		}
		is_success = true;
	}
	return is_success;
}

bool ResManager::AsyncAccept(TcpLink link, TcpSocket* socket, AcceptBuff* buffer) {
	auto accept_socket = new TcpSocket;
	if (!accept_socket->Create(socket->callback())) {
		delete accept_socket;
		return false;
	}
	buffer->set_link(link);
	buffer->set_accept_socket(accept_socket);
	if (!socket->AsyncAccept(accept_socket->socket(), buffer->buffer(), (LPOVERLAPPED)buffer)) {
		delete accept_socket;
		tcp_buff_pool_.ReturnAcceptBuff(buffer);
		return false;
	}
	return true;
}

bool ResManager::AsyncTcpRecv(TcpLink link, TcpSocket* socket, TcpRecvBuff* buffer) {
	buffer->set_link(link);
	if (!socket->AsyncRecv(buffer->buffer(), buffer->buffer_size(), (LPOVERLAPPED)buffer)) {
		OnTcpRecvNothing(link, buffer, socket->callback());
		return false;
	}
	return true;
}

void ResManager::OnTcpRecvNothing(TcpLink link, TcpRecvBuff* buffer, NetInterface* callback) {
	tcp_buff_pool_.ReturnRecvBuff(buffer);
	RemoveTcpLink(link);
	callback->OnTcpLinkDisconnected(link);
}

void ResManager::OnTcpRecvError(TcpLink link, TcpRecvBuff* buffer, NetInterface* callback) {
	tcp_buff_pool_.ReturnRecvBuff(buffer);
	RemoveTcpLink(link);
	callback->OnTcpLinkError(link, 1);
}

bool ResManager::AsyncUdpRecv(UdpLink link, UdpSocket* socket, UdpRecvBuff* buffer) {
	buffer->set_link(link);
	if (!socket->AsyncRecvFrom(buffer->buffer(), buffer->buffer_size(), (LPOVERLAPPED)buffer, buffer->from_addr())) {
		OnUdpRecvError(link, buffer, socket->callback());
		return false;
	}
	return true;
}

void ResManager::OnUdpRecvError(UdpLink link, UdpRecvBuff* buffer, NetInterface* callback) {
	udp_buff_pool_.ReturnRecvBuff(buffer);
	RemoveUdpLink(link);
	callback->OnUdpLinkError(link, 1);
}

bool ResManager::TransferAsyncType(LPOVERLAPPED ovlp, DWORD transfer_size) {
	BaseBuffer* async_buffer = (BaseBuffer*)ovlp;
	switch (async_buffer->async_type()) {
  case kAsyncTypeAccept:
		return OnTcpLinkAccept((AcceptBuff*)async_buffer, transfer_size);
	case kAsyncTypeTcpSend:
		return OnTcpLinkSend((TcpSendBuff*)async_buffer, transfer_size);
	case kAsyncTypeTcpRecv:
		return OnTcpLinkRecv((TcpRecvBuff*)async_buffer, transfer_size);
	case kAsyncTypeUdpSend:
		return OnUdpLinkSend((UdpSendBuff*)async_buffer, transfer_size);
	case kAsyncTypeUdpRecv:
		return OnUdpLinkRecv((UdpRecvBuff*)async_buffer, transfer_size);
	default:
		return false;
	}
}

bool ResManager::OnTcpLinkAccept(AcceptBuff* buffer, int accept_size) {
	auto listen_link = buffer->link();
	auto listen_socket = FindTcpSocket(listen_link);
	if (listen_socket == nullptr) {
		tcp_buff_pool_.ReturnAcceptBuff(buffer);
		return false;
	}
	auto accept_socket = (TcpSocket*)(buffer->accept_socket());
	auto accept_link = AddTcpLink(accept_socket);
	accept_socket->SetAccepted(listen_socket->socket());
	iocp_.BindToIOCP(accept_socket->socket());
	auto callback = accept_socket->callback();
	callback->OnTcpLinkAccepted(listen_link, accept_link);
	auto recv_buffer = tcp_buff_pool_.GetRecvBuffer();
	if (recv_buffer == nullptr) {
		RemoveTcpLink(accept_link);
		callback->OnTcpLinkError(accept_link, 2);
	} else {
		AsyncTcpRecv(accept_link, accept_socket, recv_buffer);
	}
	buffer->ResetBuffer();
	return AsyncAccept(listen_link, listen_socket, buffer);
}

bool ResManager::OnTcpLinkSend(TcpSendBuff* buffer, int send_size) {
	tcp_buff_pool_.ReturnSendBuff(buffer);
	return true;
}

bool ResManager::OnTcpLinkRecv(TcpRecvBuff* buffer, int recv_size) {
  auto recv_link = buffer->link();
	auto recv_socket = FindTcpSocket(recv_link);
	if (recv_socket == nullptr) {
		tcp_buff_pool_.ReturnRecvBuff(buffer);
		return false;
	}
	auto callback = recv_socket->callback();
	if (recv_size == 0) {// 接收到0字节表明对方断链了
		OnTcpRecvNothing(recv_link, buffer, callback);
		return true;
	}
	auto recv_buff = buffer->buffer();
  auto total_dealed = 0;
  while (total_dealed < recv_size) {
    auto current_dealed = 0;
    if (!ParseHeader(recv_socket, &recv_buff[total_dealed], recv_size - total_dealed, current_dealed)) {
      OnTcpRecvError(recv_link, buffer, callback);
			return false;
    }
    total_dealed += current_dealed;
		if (total_dealed >= recv_size) {
			break;
		}
    current_dealed = ParsePacket(recv_socket, &recv_buff[total_dealed], recv_size - total_dealed);
    total_dealed += current_dealed;
  }
  auto& all_packet = recv_socket->all_packets();
  for (const auto& i : all_packet) {
    callback->OnTcpLinkReceived(recv_link, &i[0], i.size());
  }
  all_packet.clear();
	return AsyncTcpRecv(recv_link, recv_socket, buffer);
}

unsigned int ResManager::GetPacketSize(const std::vector<char>& header_stream) {
	int packet_size = 0;
	if (!header_stream.empty()) {
		TcpHeader header;
		if (!header.Init(&header_stream[0], header_stream.size())) {
			return 0;
		}
		packet_size = header.packet_size();
	}
  return packet_size;
}

bool ResManager::ParseHeader(TcpSocket* recv_sock, const char* data, int size, int& dealed_size) {
  auto& header = recv_sock->current_header();
  auto& packet = recv_sock->current_packet();
  if (header.size() < kTcpHeadSize) {// 还在处理TCP头
    int left_header_size = kTcpHeadSize - header.size();  // TCP头还欠多少没收完
    if (left_header_size > size) {// 剩余的流中也不够填满TCP头
      for (auto i = 0; i< size; ++i) {
        header.push_back(data[i]);
      }
      dealed_size = size;
    } else {// 剩余的流中足够填满TCP头
      for (auto i = 0; i< left_header_size; ++i) {
        header.push_back(data[i]);
      }
      dealed_size = left_header_size;
      packet.clear();  // 完整接收到TCP头，开始了一个新包，原包清零
			auto packet_size = GetPacketSize(header);
			if (packet_size == 0) {
				return false;
			}
			packet.reserve(packet_size);
    }
	}
  return true;
}

int ResManager::ParsePacket(TcpSocket* recv_sock, const char* data, int size) {
  auto& header = recv_sock->current_header();
  auto& packet = recv_sock->current_packet();
  auto& all_packets = recv_sock->all_packets();
	auto packet_size = GetPacketSize(header);
  if (packet.size() < packet_size) {// 还在处理一个包
    int left_packet_size = packet_size - packet.size();  // 包还欠多少没收完
    if (left_packet_size > size) {// 剩余的流中也不够填满这个包
      for (auto i = 0; i< size; ++i) {
        packet.push_back(data[i]);
      }
			return size;
    } else {
      for (auto i = 0; i< left_packet_size; ++i) {
        packet.push_back(data[i]);
      }
      all_packets.push_back(std::move(packet));
      header.clear();  // 完整接收到一个包，开始下一个TCP头，原TCP头清零
			return left_packet_size;
    }
  }
  return 0;
}

bool ResManager::OnUdpLinkSend(UdpSendBuff* buffer, int send_size) {
	udp_buff_pool_.ReturnSendBuff(buffer);
	return true;
}

bool ResManager::OnUdpLinkRecv(UdpRecvBuff* buffer, int recv_size) {
	auto recv_link = buffer->link();
	auto recv_socket = FindUdpSocket(recv_link);
	if (recv_socket == nullptr) {
		udp_buff_pool_.ReturnRecvBuff(buffer);
		return false;
	}
	auto callback = recv_socket->callback();
	std::string ip;
	int port = 0;
	recv_socket->FromSockAddr(*buffer->from_addr(), ip, port);
	callback->OnUdpLinkReceived(recv_link, buffer->buffer(), recv_size, ip.c_str(), port);
	return AsyncUdpRecv(recv_link, recv_socket, buffer);
}

} // namespace net