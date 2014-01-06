#ifndef NET_RES_MANAGER_H_
#define NET_RES_MANAGER_H_

#include "iocp.h"
#include "net.h"
#include "tcp_buff_pool.h"
#include "tcp_socket.h"
#include "udp_buff_pool.h"
#include "udp_socket.h"
#include "singleton.h"
#include <map>
#include <memory>

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
  bool TcpCreate(NetInterface* callback, const char* ip, int port, TcpHandle& new_handle);
  bool TcpDestroy(TcpHandle handle);
  bool TcpListen(TcpHandle handle);
  bool TcpConnect(TcpHandle handle, const char* ip, int port);
  bool TcpSend(TcpHandle handle, const char* packet, int size);
  bool TcpGetLocalAddr(TcpHandle handle, char ip[16], int& port);
  bool TcpGetRemoteAddr(TcpHandle handle, char ip[16], int& port);
  bool UdpCreate(NetInterface* callback, const char* ip, int port, UdpHandle& new_handle);
  bool UdpDestroy(UdpHandle handle);
  bool UdpSendTo(UdpHandle handle, const char* packet, int size, const char* ip, int port);

 private:
  ResManager(const ResManager&);	// singleton: can not be copied nor assigned
  ResManager& operator=(const ResManager&);

  bool NewTcpSocket(TcpHandle& new_handle, const std::shared_ptr<TcpSocket>& new_socket);
  bool NewUdpSocket(TcpHandle& new_handle, const std::shared_ptr<UdpSocket>& new_socket);
  void RemoveTcpSocket(TcpHandle handle);
  void RemoveUdpSocket(UdpHandle handle);
  std::shared_ptr<TcpSocket> GetTcpSocket(TcpHandle handle);
  std::shared_ptr<UdpSocket> GetUdpSocket(UdpHandle handle);

  bool AsyncMoreTcpAccept(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, int count);
  bool AsyncTcpAccept(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, TcpAcceptBuff* buffer);
  bool AsyncTcpRecv(TcpHandle handle, const std::shared_ptr<TcpSocket>& socket, TcpRecvBuff* buffer);
  bool AsyncMoreUdpRecv(UdpHandle handle, const std::shared_ptr<UdpSocket>& socket, int count);
  bool AsyncUdpRecv(UdpHandle handle, const std::shared_ptr<UdpSocket>& socket, UdpRecvBuff* buffer);

  bool TransferAsyncType(LPOVERLAPPED ovlp, DWORD transfer_size);
  bool OnTcpAccept(TcpAcceptBuff* buffer);
  bool OnTcpSend(TcpSendBuff* buffer);
  bool OnTcpRecv(TcpRecvBuff* buffer, int recv_size);
  bool OnUdpSend(UdpSendBuff* buffer);
  bool OnUdpRecv(UdpRecvBuff* buffer, int recv_size);

  bool OnTcpAcceptNew(TcpHandle listen_handle, const std::shared_ptr<TcpSocket>& listen_socket, const std::shared_ptr<TcpSocket>& accept_socket);
  void OnTcpError(TcpHandle handle, NetInterface* callback, int error);
  void OnUdpError(UdpHandle handle, NetInterface* callback, int error);

 private:
  bool net_started_;
  IOCP iocp_;
  TcpBuffPool tcp_buff_pool_;
  std::mutex tcp_socket_lock_;
  unsigned long tcp_socket_count_;
  std::map<TcpHandle, std::shared_ptr<TcpSocket>> tcp_socket_;
  UdpBuffPool udp_buff_pool_;
  std::mutex udp_socket_lock_;
  unsigned long udp_socket_count_;
  std::map<UdpHandle, std::shared_ptr<UdpSocket>> udp_socket_;
};

typedef utility::Singleton<ResManager> SingleResManager;

} // namespace net

#endif	// NET_RES_MANAGER_H_