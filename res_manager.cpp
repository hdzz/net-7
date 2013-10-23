#include "res_manager.h"
#include <cassert>

namespace net {

ResManager::ResManager() {
  is_net_started_ = false;
  tcp_link_count_ = 0;
  iocp_ = NULL;
}

ResManager::~ResManager() {
  CleanupNet();
}

bool ResManager::StartupNet(StartupType type) {
	if (is_net_started_) {
		return true;
	}
	WSAData data;
	if (::WSAStartup(MAKEWORD(2,2), &data) != 0) {
		return false;
	}
  is_net_started_ = true;
	if (!tcp_buff_pool_.Init()) {
    CleanupNet();
		return false;
	}
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (iocp_ == NULL) {
		CleanupNet();
		return false;
	}
	auto thread_num = GetProcessorNum() * 2;
	for (auto i=0; i<thread_num; ++i) {
		auto thread_proc = std::bind(&ResManager::IocpWorker, this);
    auto new_thread = new std::thread(thread_proc);
		iocp_thread_.push_back(new_thread);
	}
	return true;
}

bool ResManager::CleanupNet() {
	if (!is_net_started_) {
		return true;
	}
	for (const auto& i : iocp_thread_) {
		::PostQueuedCompletionStatus(iocp_, 0, NULL, NULL);
	}
  for (auto& i : iocp_thread_) {
    i->join();
    delete i;
  }
	iocp_thread_.clear();
  do {
    std::lock_guard<std::mutex> lock(tcp_link_lock_);
    for (const auto& i : tcp_link_) {
      delete i.second;
    }
    tcp_link_.clear();
  } while (false);
  tcp_buff_pool_.Clear();
  ::CloseHandle(iocp_);
  iocp_ = NULL;
  ::WSACleanup();
  tcp_link_count_ = 0;
  is_net_started_ = false;
	return true;
}

bool ResManager::CreateTcpLink(NetInterface* callback, const char* ip, int port, TcpLink& new_link) {
	if (callback == nullptr) {
		return false;
	}
	auto new_socket = new TcpSocket(callback);
	if (!new_socket->Create()) {
		delete new_socket;
		return false;
	}
  if (!ip) {
    ip = "0.0.0.0";
  }
	if (!new_socket->Bind(ip, port)) {
		delete new_socket;
		return false;
	}
	if (!BindToIOCP(new_socket)) {
		delete new_socket;
		return false;
	}
	new_link = AddTcpLink(new_socket);
	return true;
}

bool ResManager::DestroyTcpLink(TcpLink link) {
	return RemoveTcpLink(link);
}

bool ResManager::Listen(TcpLink link, int backlog) {
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
  if (backlog > kAcceptBuffPoolSize) {
    backlog = kAcceptBuffPoolSize;
  }
	if (!find_socket->Listen(backlog)) {
		return false;
	}
	return AsyncMoreAccept(link, find_socket, backlog);
}

bool ResManager::Connect(TcpLink link, const char* ip, int port) {
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	if (!find_socket->Connect(ip, port)) {
		return false;
	}
  auto buffer = tcp_buff_pool_.GetRecvBuffer();
  if (buffer == nullptr) {
    return false;
  }
	return AsyncTcpRecv(link, find_socket, buffer);
}

bool ResManager::SendTcpPacket(TcpLink link, const char* packet, int size) {
	if (packet == nullptr || size <= 0 || size > kMaxTcpPacketSize) {
		return false;
	}
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	auto buffer = tcp_buff_pool_.GetSendBuffer(packet, size);
	for (auto& i : buffer) {
		i->set_link(link);
		i->set_socket(find_socket);
		i->set_async_type(kAsyncTypeTcpSend);
		if (!find_socket->AsyncSend(const_cast<char*>(i->buffer()), i->buffer_size(), (LPOVERLAPPED)i)) {
				tcp_buff_pool_.ReturnTcpSendBuff(i);
				return false;
		}
	}
	return true;
}

bool ResManager::GetTcpLinkAddr(TcpLink link, EitherPeer type, char ip[16], int& port) {
	if (ip == nullptr) {
		return false;
	}
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
  std::string ip_string;
  if (type == EitherPeer::kLocal) {
    find_socket->attribute().GetLocalAddr(ip_string, port);
  } else if (type == EitherPeer::kRemote) {
    find_socket->attribute().GetRemoteAddr(ip_string, port);
  } else {
    return false;
  }
  strcpy_s(ip, 16, ip_string.c_str());
	return true;
}

bool ResManager::SetTcpLinkAttr(TcpLink link, AttributeType type, int value) {
	
	return true;
}

bool ResManager::GetTcpLinkAttr(TcpLink link, AttributeType type, int& value) {
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
    return false;
  }
  if (type == AttributeType::kSendSpeed) {
    value = find_socket->attribute().GetSendSpeed();
  } else if (type == AttributeType::kRecvSpeed) {
    value = find_socket->attribute().GetRecvSpeed();
  } else {
    return false;
  }
	return true;
}

bool ResManager::CreateUdpLink(NetInterface* callback, const char* ip, int port, bool broadcast, UdpLink& new_link) {

	return true;
}

bool ResManager::DestroyUdpLink(UdpLink link) {

	return true;
}

bool ResManager::SendUdpPacket(UdpLink link, const char* packet, int size, const char* ip, int port) {

	return true;
}

int ResManager::GetProcessorNum() {
	SYSTEM_INFO info;
	::GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
}

bool ResManager::BindToIOCP(TcpSocket* socket) {
	HANDLE existing_iocp = ::CreateIoCompletionPort((HANDLE)socket->socket(), iocp_, NULL, 0);
	if (existing_iocp != iocp_) {
		return false;
	}
	return true;
}

TcpLink ResManager::AddTcpLink(TcpSocket* socket) {
	std::lock_guard<std::mutex> lock(tcp_link_lock_);
	TcpLink new_link = ++tcp_link_count_;
	tcp_link_.insert(std::make_pair(new_link, socket));
	return new_link;
}

bool ResManager::RemoveTcpLink(TcpLink link) {
	std::lock_guard<std::mutex> lock(tcp_link_lock_);
	auto find_link = tcp_link_.find(link);
	if (find_link == tcp_link_.end()) {
		return false;
	}
	auto find_socket = find_link->second;
	delete find_socket;
	tcp_link_.erase(find_link);
	return true;
}

TcpSocket* ResManager::FindTcpSocket(TcpLink link) {
	std::lock_guard<std::mutex> lock(tcp_link_lock_);
	auto find_link = tcp_link_.find(link);
	if (find_link == tcp_link_.end()) {
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
			tcp_buff_pool_.ReturnAcceptBuff(accept_buffer);
			continue;
		}
		is_success = true;
	}
	return is_success;
}

bool ResManager::AsyncAccept(TcpLink link, TcpSocket* socket, AcceptBuff* buffer) {
	auto accept_socket = new TcpSocket(socket->callback());
	if (!accept_socket->Create()) {
		delete accept_socket;
		return false;
	}
	auto accept_link = AddTcpLink(accept_socket);
	buffer->set_link(link);
	buffer->set_socket(socket);
	buffer->set_accept_link(accept_link);
	buffer->set_accept_socket(accept_socket);
	buffer->set_async_type(kAsyncTypeAccept);
	if (!accept_socket->AsyncAccept(socket->socket(), const_cast<char*>(buffer->buffer()), (LPOVERLAPPED)buffer)) {
		return false;
	}
	return true;
}


bool ResManager::AsyncTcpRecv(TcpLink link, TcpSocket* socket, TcpRecvBuff* buffer) {
	buffer->set_link(link);
	buffer->set_socket(socket);
	buffer->set_async_type(kAsyncTypeTcpRecv);
	if (!socket->AsyncRecv(const_cast<char*>(buffer->buffer()), buffer->buffer_size(), (LPOVERLAPPED)buffer)) {
		return false;
	}
	return true;
}

bool ResManager::IocpWorker() {
	while (true) {
		DWORD transfer_size = 0;
		LPOVERLAPPED ovlp = NULL;
		ULONG completion_key = NULL;
		if (!::GetQueuedCompletionStatus(iocp_, &transfer_size, &completion_key, &ovlp, INFINITE)) {
			continue;
		}
		if (transfer_size == 0 && ovlp == NULL) {
			break;	// 这种情况只有在接收到线程退出指令时才会发生，因为以前的操作保证了ovlp不为空
		}
		BaseBuffer* async_buffer = (BaseBuffer*)ovlp;
		TransferAsyncType(async_buffer, transfer_size);
	}
	return true;
}

bool ResManager::TransferAsyncType(BaseBuffer* async_buffer, int transfer_size) {
	switch (async_buffer->async_type()) {
  case kAsyncTypeAccept:
		return OnTcpLinkAccept((AcceptBuff*)async_buffer, transfer_size);
	case kAsyncTypeTcpSend:
		return OnTcpLinkSend((TcpSendBuff*)async_buffer, transfer_size);
	case kAsyncTypeTcpRecv:
		return OnTcpLinkRecv((TcpRecvBuff*)async_buffer, transfer_size);
	case kAsyncTypeUdpSend:
		return OnUdpLinkSend(async_buffer, transfer_size);
	case kAsyncTypeUdpRecv:
		return OnUdpLinkRecv(async_buffer, transfer_size);
	default:
		return false;
	}
}

bool ResManager::OnTcpLinkAccept(AcceptBuff* buffer, int accept_size) {
	auto listen_link = buffer->link();
	auto listen_socket = (TcpSocket*)buffer->socket();
	auto accept_link = buffer->accept_link();
	auto accept_socket = (TcpSocket*)buffer->accept_socket();
	accept_socket->SetAccepted();
	buffer->ResetBuffer();
	AsyncAccept(listen_link, listen_socket, buffer);
	auto callback = accept_socket->callback();
	callback->OnTcpLinkAccepted(listen_link, accept_link);
	BindToIOCP(accept_socket);
  auto recv_buffer = tcp_buff_pool_.GetRecvBuffer();
  if (recv_buffer == nullptr) {
    return false;
  }
	return AsyncTcpRecv(accept_link, accept_socket, recv_buffer);
}

bool ResManager::OnTcpLinkSend(TcpSendBuff* buffer, int send_size) {
	assert(buffer->buffer_size() == send_size);
	auto send_socket = (TcpSocket*)buffer->socket();
	send_socket->attribute().AddSendByte(send_size);
	tcp_buff_pool_.ReturnTcpSendBuff(buffer);
	return true;
}

bool ResManager::OnTcpLinkRecv(TcpRecvBuff* buffer, int recv_size) {
  auto recv_link = buffer->link();
	auto recv_socket = (TcpSocket*)buffer->socket();
	TcpSocketAttr& attribute = recv_socket->attribute();
	attribute.AddRecvByte(recv_size);
	auto callback = recv_socket->callback();
	auto recv_buff = buffer->buffer();
  auto total_dealed = 0;
  while (total_dealed < recv_size) {
    auto current_dealed = 0;
    if (!ParseHeader(recv_socket, &recv_buff[total_dealed], recv_size - total_dealed, current_dealed)) {
      break;  // 剩下的流里面连一个TCP头都不完整，跳出循环
    }
    total_dealed += current_dealed; // 统计处理了多少
    current_dealed = 0;
    if (!ParsePacket(recv_socket, &recv_buff[total_dealed], recv_size - total_dealed, current_dealed)) {
      break;  // 剩下的流里面是个不完整的包，跳出循环
    }
    total_dealed += current_dealed; // 统计处理了多少
  }
  AsyncTcpRecv(recv_link, recv_socket, buffer);  // 继续投递接收块
  auto& all_packet = recv_socket->all_packets();
  for (const auto& i : all_packet) {// 回调
    callback->OnTcpLinkReceived(recv_link, &i[0], i.size());
  }
  all_packet.clear();
	return true;
}

unsigned int ResManager::GetPacketSize(TcpSocket* recv_sock) {
  unsigned int packet_size = 0;
  const auto& header = recv_sock->current_header();
  if (header.size() == kTcpHeadSize) {
    memcpy_s(&packet_size, sizeof(packet_size), &header[4], 4);
  }
  return packet_size;
}

bool ResManager::ParseHeader(TcpSocket* recv_sock, const char* data, int size, int& dealed_size) {
  if (data == nullptr || size == 0) {
    return false;
  }
  auto& header = recv_sock->current_header();
  auto& packet = recv_sock->current_packet();
  if (header.size() < kTcpHeadSize) {// 还在处理TCP头
    int left_header_size = kTcpHeadSize - header.size();  // TCP头还欠多少没收完
    if (left_header_size > size) {// 剩余的流中也不够填满TCP头，返回失败
      for (auto i = 0; i< size; ++i) {
        header.push_back(data[i]);
      }
      dealed_size = size;
      return false;
    } else {// 剩余的流中足够填满TCP头，返回成功
      for (auto i = 0; i< left_header_size; ++i) {
        header.push_back(data[i]);
      }
      dealed_size = left_header_size;
      packet.clear();  // 完整接收到TCP头，开始了一个新包，原包清零
      packet.reserve(GetPacketSize(recv_sock));
      return true;
    }
  }
  return true;
}

bool ResManager::ParsePacket(TcpSocket* recv_sock, const char* data, int size, int& dealed_size) {
  if (data == nullptr || size == 0) {
    return false;
  }
  auto packet_size = GetPacketSize(recv_sock);
  auto& header = recv_sock->current_header();
  auto& packet = recv_sock->current_packet();
  auto& all_packets = recv_sock->all_packets();
  if (packet.size() < packet_size) {// 还在处理一个包
    int left_packet_size = packet_size - packet.size();  // 包还欠多少没收完
    if (left_packet_size > size) {// 剩余的流中也不够填满这个包，返回失败
      for (auto i = 0; i< size; ++i) {
        packet.push_back(data[i]);
      }
      dealed_size = size;
      return false;
    } else {
      for (auto i = 0; i< left_packet_size; ++i) {
        packet.push_back(data[i]);
      }
      dealed_size = left_packet_size;
      all_packets.push_back(std::move(packet));
      header.clear();  // 完整接收到一个包，开始下一个TCP头，原TCP头清零
      return true;
    }
  }
  return true;
}

bool ResManager::OnUdpLinkSend(BaseBuffer* buffer, int send_size) {

	return true;
}

bool ResManager::OnUdpLinkRecv(BaseBuffer* buffer, int recv_size) {

	return true;
}

} // namespace net