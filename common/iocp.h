#ifndef NET_IOCP_H_
#define NET_IOCP_H_

#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <WinSock2.h>

namespace net {

class IOCP {
 public:
	IOCP();
	~IOCP();
	bool Init(std::function<bool (LPOVERLAPPED, DWORD)> callback);	// 初始化和反初始化
	void Uninit();
	bool BindToIOCP(SOCKET socket);	// 绑定到IO完成端口

 private:
	IOCP(const IOCP&);			// 不可拷贝，不可赋值
	IOCP& operator=(const IOCP&);
	bool ThreadWorker();		// 工作线程
	int GetProcessorNum();	// 获取处理器个数

 private:
	bool is_init_;																			// 是否已经初始化
	HANDLE iocp_;																				// IOCP内核对象句柄
	std::function<bool(LPOVERLAPPED, DWORD)> callback_;	// 异步处理完成之后的回调接口
	std::vector<std::thread*> iocp_thread_;							// 工作线程句柄
};

} // namespace net

#endif	// NET_IOCP_H_