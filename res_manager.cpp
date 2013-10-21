#include "res_manager.h"
#include "tcp_buff_helper.h"

namespace net {

ResManager::ResManager() { ResetMember(); }

ResManager::~ResManager() { CleanupNet(); }

void ResManager::ResetMember() {
	is_net_started_ = false;
	each_link_async_num_ = 0;
	tcp_link_count_ = 0;
	iocp_ = NULL;
}

bool ResManager::InitMemoryPool() {
	for (int i=0; i<kSendBufferNum; ++i) {
		auto send_buffer = new TcpSendBuff;
		tcp_send_buff_.push_back(send_buffer);
	}
	for (int i=0; i<kRecvBufferNum; ++i) {
		auto recv_buffer = new TcpRecvBuff;
		tcp_recv_buff_.push_back(recv_buffer);
	}
	for (int i=0; i<kAcceptBufferNum; ++i) {
		auto accept_buffer = new AcceptBuff;
		accept_buff_.push_back(accept_buffer);
	}
	return true;
}

void ResManager::ClearMemoryPool() {
	do {
		ThreadLock<CriticalSection> lock(tcp_link_lock_);
		for (auto i : tcp_link_) { delete i.second; }
		tcp_link_.clear();
	} while (false);
	do {
		ThreadLock<CriticalSection> lock(tcp_send_buff_lock_);
		for (auto i : tcp_send_buff_) { delete i; }
		tcp_send_buff_.clear();
	} while (false);
	do {
		ThreadLock<CriticalSection> lock(tcp_recv_buff_lock_);
		for (auto i : tcp_recv_buff_) { delete i; }
		tcp_recv_buff_.clear();
	} while (false);
	do {
		ThreadLock<CriticalSection> lock(accept_buff_lock_);
		for (auto i : accept_buff_) { delete i; }
		accept_buff_.clear();
	} while (false);
}

bool ResManager::StartupNet(StartupType type) {
	if (is_net_started_) {
		return true;
	}
	WSAData data;
	if (WSAStartup(MAKEWORD(2,2), &data) != 0) {
		return false;
	}
	if (!InitMemoryPool()) {
		return false;
	}
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (iocp_ == NULL) {
		ClearMemoryPool();
		return false;
	}
	is_net_started_ = true;
	int thread_num = GetProcessorNum() * 2;
	worker_thread_.reserve(thread_num);
	for (int i=0; i<thread_num; ++i) {
		HANDLE worker_thread = ::CreateThread(NULL, 0, IocpWorkerThread, this, 0, NULL);
		if (worker_thread == NULL) {
			CleanupNet();
			return false;
		}
		worker_thread_.push_back(worker_thread);
	}
	each_link_async_num_ = thread_num;
	return true;
}

bool ResManager::CleanupNet() {
	if (!is_net_started_) {
		return true;
	}
	for (auto i : worker_thread_) {
		::PostQueuedCompletionStatus(iocp_, 0, NULL, NULL);
	}
	::WaitForMultipleObjects(worker_thread_.size(), &worker_thread_[0], TRUE, INFINITE);
	for (auto i: worker_thread_) {
		::CloseHandle(i);
	}
	worker_thread_.clear();
	::CloseHandle(iocp_);
	ClearMemoryPool();
	ResetMember();
	WSACleanup();
	return true;
}

bool ResManager::CreateTcpLink(NetInterface* callback, TcpLink& new_link, const char* ip, int port) {
	if (callback == nullptr) {
		return false;
	}
	auto new_socket = new TcpSocket(callback);
	if (!new_socket->Create()) {
		delete new_socket;
		return false;
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

bool ResManager::Listen(TcpLink link) {
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	if (!find_socket->Listen()) {
		return false;
	}
	return AsyncMoreAccept(link, find_socket);
}

bool ResManager::Connect(TcpLink link, const char* ip, int port) {
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	if (!find_socket->Connect(ip, port)) {
		return false;
	}
	return AsyncMoreTcpRecv(link, find_socket);
}

bool ResManager::SendTcpPacket(TcpLink link, const char* packet, int size) {
	if (packet == nullptr || size <= 0 || size > kMaxTcpPacketSize) {
		return false;
	}
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	vector<TcpSendBuff*> buffer;
	int buffer_num = CalcSendBuffNum(size);
	if (!GetAllTcpSendBuff(buffer, buffer_num)) {
		return false;
	}
	int packet_count = find_socket->attribute().AddPacketCount();
	FillTcpSendBuff(packet_count, packet, size, buffer);
	for (auto i : buffer) {
		i->set_link(link);
		i->set_socket(find_socket);
		i->set_async_type(kAsyncTypeTcpSend);
		if (!find_socket->AsyncSend(const_cast<char*>(i->buffer()), i->buffer_size(), (LPOVERLAPPED)i)) {
				ReturnTcpSendBuff(i);
				return false;
		}
	}
	return true;
}

bool ResManager::GetTcpLinkAddr(TcpLink link, EitherPeer type, char* ip, int ip_size, int& port) {
	if (ip == nullptr || ip_size == 0) {
		return false;
	}
	auto find_socket = FindTcpSocket(link);
	if (find_socket == nullptr) {
		return false;
	}
	string ip_string;
	if (type == EitherPeer::kLocal) {
		find_socket->attribute().GetLocalAddr(ip_string, port);
	} else if (type == EitherPeer::kRemote) {
		find_socket->attribute().GetRemoteAddr(ip_string, port);
	} else {
		return false;
	}
	if (ip_size <= static_cast<int>(ip_string.size())) {
		return false;
	}
	strcpy_s(ip, ip_size, ip_string.c_str());
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

bool ResManager::CreateUdpLink(NetInterface* callback, UdpLink& new_link, const char* ip, int port, bool broadcast) {

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
	ThreadLock<CriticalSection> lock(tcp_link_lock_);
	TcpLink new_link = ++tcp_link_count_;
	tcp_link_.insert(make_pair(new_link, socket));
	return new_link;
}

bool ResManager::RemoveTcpLink(TcpLink link) {
	ThreadLock<CriticalSection> lock(tcp_link_lock_);
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
	ThreadLock<CriticalSection> lock(tcp_link_lock_);
	auto find_link = tcp_link_.find(link);
	if (find_link == tcp_link_.end()) {
		return nullptr;
	}
	return find_link->second;
}

bool ResManager::GetAllTcpSendBuff(vector<TcpSendBuff*>& buffer, int buffer_num) {
	buffer.reserve(buffer_num);
	bool is_buff_enough = true;
	for (int i=0; i<buffer_num; ++i) {
		auto send_buffer = GetTcpSendBuffer();
		if (send_buffer == nullptr) {
			is_buff_enough = false;
			break;
		}
		buffer.push_back(send_buffer);
	}
	if (!is_buff_enough) {
		for (auto i : buffer) {
			ReturnTcpSendBuff(i);
		}
		buffer.clear();
	}
	return is_buff_enough;
}

bool ResManager::AsyncMoreAccept(TcpLink link, TcpSocket* socket) {
	bool is_success = false;
	for (int i=0; i<each_link_async_num_; ++i) {
		auto accept_buffer = GetAcceptBuffer();
		if (accept_buffer == nullptr) {
			continue;
		}
		if (!AsyncAccept(link, socket, accept_buffer)) {
			ReturnAcceptBuff(accept_buffer);
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
	if (!accept_socket->AsyncAccept(socket->socket(), (PVOID)buffer->buffer(), (LPOVERLAPPED)buffer)) {
		return false;
	}
	return true;
}

bool ResManager::AsyncMoreTcpRecv(TcpLink link, TcpSocket* socket) {
	bool is_success = false;
	for (int i=0; i<each_link_async_num_; ++i) {
		auto recv_buffer = GetTcpRecvBuffer();
		if (recv_buffer == nullptr) {
			continue;
		}
		if (!AsyncTcpRecv(link, socket, recv_buffer)) {
			ReturnTcpRecvBuff(recv_buffer);
			continue;
		}
		is_success = true;
	}
	return is_success;
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



DWORD WINAPI ResManager::IocpWorkerThread(LPVOID parameter) {
	ResManager* manager = (ResManager*)parameter;
	if (manager != nullptr) {
		manager->IocpWorker();
	}
	return 0;
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
	NetInterface* callback = accept_socket->callback();
	callback->OnTcpLinkAccept(listen_link, accept_link);
	BindToIOCP(accept_socket);
	return AsyncMoreTcpRecv(accept_link, accept_socket);
}

bool ResManager::OnTcpLinkSend(TcpSendBuff* buffer, int send_size) {
	if (buffer->buffer_size() != send_size) {
		// 由于系统发送缓冲已经被设置为0，所以发送必定是完全的，这种情况理论上是不存在的
	}
	auto send_socket = (TcpSocket*)buffer->socket();
	send_socket->attribute().AddSendByte(send_size);
	ReturnTcpSendBuff(buffer);
	return true;
}

bool ResManager::OnTcpLinkRecv(TcpRecvBuff* buffer, int recv_size) {
	auto recv_socket = (TcpSocket*)buffer->socket();
	TcpSocketAttr& attribute = recv_socket->attribute();
	attribute.AddRecvByte(recv_size);
	auto callback = recv_socket->callback();
	auto recv_buff = buffer->buffer();
	int deal_size = 0;				// 已处理完多少接收缓冲的数据量
	int left_size = recv_size;// 未处理完多少接收缓冲数据量
	vector<TcpRecvingPkt*> callback_packet;	// 所有接收完全的发送包

	do {
		ThreadLock<CriticalSection> lock(attribute.recv_lock_);
		while (left_size > 0) {// while循环找到每个发送缓冲（或完整或不完整，但起码具备开头或者结尾）
			const char* each_buff = nullptr;	// 每个缓冲
			int each_buff_size = 0;						// 每个缓冲大小
			TcpRecvingPkt* packet = nullptr;	// 每个缓冲对应的包对象
			if (attribute.last_recv_buff_ == -1) {// 新的发送缓冲，然后需要关心的是该缓冲完没完
				// 拷贝TCP缓冲头
				TcpBuffHead head;
				memcpy(&head, recv_buff+deal_size, kTcpBuffHeadSize);
				// 该缓冲开始指针
				each_buff = recv_buff + deal_size + kTcpBuffHeadSize;
				// 找此缓冲对应的包对象
				packet = attribute.GetRecvingPkt(head.sequence_, head.total_buff_);
				// 包中最后一个缓冲
				if (head.total_buff_ == head.this_buff_ + 1) {
					packet->packet_size_ = (head.total_buff_-1) * 65528;
					packet->packet_size_ += head.data_size_;
				}
				// 计算此缓冲在发送时的大小
				int tail_size = kTcpBuffHeadSize - (head.data_size_ % kTcpBuffHeadSize);
				if (tail_size == kTcpBuffHeadSize) {
					tail_size = 0;
				}
				const int send_buff_size = kTcpBuffHeadSize + head.data_size_ + tail_size;
				if (send_buff_size <= left_size) {
					// 该缓冲接收完毕
					each_buff_size = head.data_size_;
					bool is_pkt_recv_done = packet->CopyBuffer(each_buff, each_buff_size, true, true, head.this_buff_);
					if (is_pkt_recv_done) {// 该包接收完毕
						callback_packet.push_back(packet);
						attribute.recving_pkt_.erase(attribute.recving_pkt_.find(head.sequence_));
					}
					deal_size += send_buff_size;
					left_size -= send_buff_size;
				} else {
					// 该缓冲还有部分未接收
					each_buff_size = left_size - kTcpBuffHeadSize;
					packet->CopyBuffer(each_buff, each_buff_size, false, true, head.this_buff_);
					packet->incomplete_offset_ = head.this_buff_ * 65528  + each_buff_size;
					packet->incomplete_left_ = head.data_size_ - each_buff_size;
					left_size = 0;
					attribute.last_recv_pkt_ = head.sequence_;
					attribute.last_recv_buff_ = head.this_buff_;
				}
			} else {// 遗留有上一个发送缓冲的数据
				packet = attribute.recving_pkt_.find((int)attribute.last_recv_pkt_)->second;
				each_buff = recv_buff;
				if (packet->incomplete_left_ > recv_size) {// 仍然未接收完
					each_buff_size = recv_size;
					packet->CopyBuffer(each_buff, each_buff_size, false, false, attribute.last_recv_buff_);
					packet->incomplete_offset_ += each_buff_size;
					packet->incomplete_left_ -= each_buff_size;
					left_size = 0;
				} else {// 终于接收完毕了
					each_buff_size = packet->incomplete_left_;
					bool is_pkt_recv_done = packet->CopyBuffer(each_buff, each_buff_size, true, false, attribute.last_recv_buff_);
					if (is_pkt_recv_done) {// 该包接收完毕
						callback_packet.push_back(packet);
						attribute.recving_pkt_.erase(attribute.recving_pkt_.find((int)attribute.last_recv_pkt_));
					}
					int tail_size = kTcpBuffHeadSize - (each_buff_size % kTcpBuffHeadSize);
					if (tail_size == kTcpBuffHeadSize) {
						tail_size = 0;
					}
					const int left_send_size = each_buff_size + tail_size;
					deal_size += left_send_size;
					left_size -= left_send_size;
					attribute.last_recv_buff_ = -1;
				}
			}
		}
	} while (false);
	for (auto i : callback_packet) {
		callback->OnTcpLinkRecv(buffer->link(), i->packet_, i->packet_size_);
		delete i;
	}
	ReturnTcpRecvBuff(buffer);
	return true;
}

bool ResManager::OnUdpLinkSend(BaseBuffer* buffer, int send_size) {

	return true;
}

bool ResManager::OnUdpLinkRecv(BaseBuffer* buffer, int recv_size) {

	return true;
}

} // namespace net