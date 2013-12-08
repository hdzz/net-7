#include "iocp.h"
#include "log.h"

namespace net {

IOCP::IOCP() {
	is_init_ = false;
	iocp_ = NULL;
}

IOCP::~IOCP() {
	Uninit();
}

bool IOCP::Init(std::function<bool(LPOVERLAPPED, DWORD)> callback) {
	if (is_init_) {
		return true;
	}
	if (!callback) {
		return false;
	}
	WSAData wsa_data = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		return false;
	}
	callback_ = callback;
	is_init_ = true;
	iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (iocp_ == NULL) {
		Uninit();
		return false;
	}
	auto thread_num = GetProcessorNum() * 2;
	auto thread_proc = std::bind(&IOCP::ThreadWorker, this);
	for (auto i = 0; i < thread_num; ++i) {
		auto new_thread = new std::thread(thread_proc);
		iocp_thread_.push_back(new_thread);
	}
	LOG(kStartup, "IOCP thread number: %d.", iocp_thread_.size());
	return true;
}

void IOCP::Uninit() {
	if (!is_init_) {
		return;
	}
	for (const auto& i : iocp_thread_) {
		::PostQueuedCompletionStatus(iocp_, 0, NULL, NULL);
	}
	for (const auto& i : iocp_thread_) {
		i->join();
		delete i;
	}
	iocp_thread_.clear();
	if (iocp_ != NULL) {
		::CloseHandle(iocp_);
		iocp_ = NULL;
	}
	::WSACleanup();
	callback_ = nullptr;
	is_init_ = false;
}

bool IOCP::BindToIOCP(SOCKET socket) {
	HANDLE existing_iocp = ::CreateIoCompletionPort((HANDLE)socket, iocp_, NULL, 0);
	if (existing_iocp != iocp_) {
		return false;
	}
	return true;
}

bool IOCP::ThreadWorker() {
	while (true) {
		DWORD transfer_size = 0;
		LPOVERLAPPED ovlp = NULL;
		ULONG completion_key = NULL;
		if (!::GetQueuedCompletionStatus(iocp_, &transfer_size, &completion_key, &ovlp, INFINITE)) {
			int error_code = ::WSAGetLastError();
			if (error_code != ERROR_NETNAME_DELETED && error_code != ERROR_CONNECTION_ABORTED) {
				LOG(kError, "GetQueuedCompletionStatus failed, error code: %d.", error_code);
				continue;
			}
		}
		if (transfer_size == 0 && ovlp == NULL) {
			break;	// 这种情况只有在接收到线程退出指令时才会发生，因为以前的操作保证了ovlp不为空
		}
		if (callback_) {
			callback_(ovlp, transfer_size);
		}
	}
	return true;
}

int IOCP::GetProcessorNum() {
	SYSTEM_INFO info;
	::GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
}

} // namespace net