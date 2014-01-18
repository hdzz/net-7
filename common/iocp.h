#ifndef NET_IOCP_H_
#define NET_IOCP_H_

#include <functional>
#include <thread>
#include <vector>
#include <WinSock2.h>

namespace net {

class IOCP {
 public:
  IOCP();
  ~IOCP();
  bool Init(std::function<bool (LPOVERLAPPED, DWORD)> callback);
  void Uninit();
  bool BindToIOCP(SOCKET socket);

 private:
  IOCP(const IOCP&) = delete;
  IOCP& operator=(const IOCP&) = delete;
  bool ThreadWorker();

 private:
  bool init_;
  HANDLE iocp_;
  std::function<bool (LPOVERLAPPED, DWORD)> callback_;
  std::vector<std::thread*> iocp_thread_;
};

} // namespace net

#endif	// NET_IOCP_H_