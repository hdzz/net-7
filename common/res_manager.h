#ifndef NET_RES_MANAGER_H_
#define NET_RES_MANAGER_H_

#include "iocp.h"
#include "net.h"
#include "tcp_buffer.h"
#include "tcp_socket.h"
#include "udp_buffer.h"
#include "udp_socket.h"
#include "singleton.h"
#include <map>

namespace net {

class ResManager {
 public:
  ResManager();
  ~ResManager();

  /************************************************************************/
  /* interface implementation                                             */
  /************************************************************************/
  bool StartupNet();
  bool CleanupNet();
  bool TcpCreate(NetInterface* callback, const std::string& ip, int port, TcpHandle& new_handle);
  bool TcpDestroy(TcpHandle handle);
  bool TcpListen(TcpHandle handle);
  bool TcpConnect(TcpHandle handle, const std::string& ip, int port);
  bool TcpSend(TcpHandle handle, std::unique_ptr<char[]> packet, int size);
  bool TcpGetLocalAddr(TcpHandle handle, char ip[16], int& port);
  bool TcpGetRemoteAddr(TcpHandle handle, char ip[16], int& port);
  bool UdpCreate(NetInterface* callback, const std::string& ip, int port, UdpHandle& new_handle);
  bool UdpDestroy(UdpHandle handle);
  bool UdpSendTo(UdpHandle handle, std::unique_ptr<char[]> packet, int size, const std::string& ip, int port);

 private:
  ResManager(const ResManager&) = delete;	// singleton: can not be copied nor assigned
  ResManager& operator=(const ResManager&) = delete;

  bool NewTcpSocket(TcpHandle& new_handle, const std::shared_ptr<TcpSocket>& new_socket);
  bool NewUdpSocket(TcpHandle& new_handle, const std::shared_ptr<UdpSocket>& new_socket);
  void RemoveTcpSocket(TcpHandle handle);
  void RemoveUdpSocket(UdpHandle handle);
  std::shared_ptr<TcpSocket> GetTcpSocket(TcpHandle handle);
  std::shared_ptr<UdpSocket> GetUdpSocket(UdpHandle handle);

  TcpAcceptBuffer* GetTcpAcceptBuffer();
  TcpSendBuffer* GetTcpSendBuffer();
  TcpRecvBuffer* GetTcpRecvBuffer();
  UdpSendBuffer* GetUdpSendBuffer();
  UdpRecvBuffer* GetUdpRecvBuffer();
  void ReturnTcpAcceptBuffer(TcpAcceptBuffer* buffer);
  void ReturnTcpSendBuffer(TcpSendBuffer* buffer);
  void ReturnTcpRecvBuffer(TcpRecvBuffer* buffer);
  void ReturnUdpSendBuffer(UdpSendBuffer* buffer);
  void ReturnUdpRecvBuffer(UdpRecvBuffer* buffer);

  bool AsyncTcpAccept(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, TcpAcceptBuffer* buffer);
  bool AsyncTcpRecv(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, TcpRecvBuffer* buffer);
  bool AsyncUdpRecv(UdpHandle handle, const std::shared_ptr<UdpSocket>& socket, UdpRecvBuffer* buffer);

  bool TransferAsyncType(LPOVERLAPPED ovlp, DWORD transfer_size);
  bool OnTcpAccept(TcpAcceptBuffer* buffer);
  bool OnTcpSend(TcpSendBuffer* buffer);
  bool OnTcpRecv(TcpRecvBuffer* buffer, int size);
  bool OnUdpSend(UdpSendBuffer* buffer);
  bool OnUdpRecv(UdpRecvBuffer* buffer, int size);

  bool OnTcpAcceptNew(TcpHandle listen_handle, const std::shared_ptr<TcpSocket>& listen_socket, const std::shared_ptr<TcpSocket>& accept_socket);
  void OnTcpError(TcpHandle handle, NetInterface* callback, int error);
  void OnUdpError(UdpHandle handle, NetInterface* callback, int error);

 private:
  bool net_started_;
  IOCP iocp_;
  std::mutex tcp_socket_lock_;
  std::mutex udp_socket_lock_;
  unsigned long tcp_socket_count_;
  unsigned long udp_socket_count_;
  std::map<TcpHandle, std::shared_ptr<TcpSocket>> tcp_socket_;
  std::map<UdpHandle, std::shared_ptr<UdpSocket>> udp_socket_;
};

typedef utility::Singleton<ResManager> SingleResManager;

} // namespace net

#endif	// NET_RES_MANAGER_H_