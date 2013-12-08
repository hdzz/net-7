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
	bool Init(std::function<bool (LPOVERLAPPED, DWORD)> callback);	// ��ʼ���ͷ���ʼ��
	void Uninit();
	bool BindToIOCP(SOCKET socket);	// �󶨵�IO��ɶ˿�

 private:
	IOCP(const IOCP&);			// ���ɿ��������ɸ�ֵ
	IOCP& operator=(const IOCP&);
	bool ThreadWorker();		// �����߳�
	int GetProcessorNum();	// ��ȡ����������

 private:
	bool is_init_;																			// �Ƿ��Ѿ���ʼ��
	HANDLE iocp_;																				// IOCP�ں˶�����
	std::function<bool(LPOVERLAPPED, DWORD)> callback_;	// �첽�������֮��Ļص��ӿ�
	std::vector<std::thread*> iocp_thread_;							// �����߳̾��
};

} // namespace net

#endif	// NET_IOCP_H_