/************************************************************************/
/*  Net Interface                                                       */
/*  THREAD: safe                                                        */
/*  AUTHOR: chen_lu@outlook.com 														            */
/************************************************************************/

#ifndef NET_INTERFACE_H_
#define NET_INTERFACE_H_

#include <memory>
#include <string>

namespace net {

typedef unsigned long TcpHandle;
typedef unsigned long UdpHandle;

const TcpHandle kInvalidTcpHandle = 0;
const UdpHandle kInvalidUdpHandle = 0;

const int kOneKibibyte = 1024;
const int kOneMebibyte = 1024 * kOneKibibyte;
const int kMaxTcpPacketSize = 16 * kOneMebibyte;
const int kMaxUdpPacketSize = 8 * kOneKibibyte;

class NetInterface {
 public:
  virtual bool OnTcpDisconnected(TcpHandle handle) = 0;
  virtual bool OnTcpAccepted(TcpHandle handle, TcpHandle accept_handle) = 0;
  virtual bool OnTcpReceived(TcpHandle handle, const char* packet, int size) = 0;
  virtual bool OnTcpError(TcpHandle handle, int error) = 0;
  virtual bool OnUdpReceived(UdpHandle handle, const char* packet, int size, const std::string& ip, int port) = 0;
  virtual bool OnUdpError(UdpHandle handle, int error) = 0;
};

#ifdef NET_EXPORTS
#define NET_API _declspec(dllexport)
#else 
#define NET_API _declspec(dllimport)
#endif // NET_EXPORTS

NET_API bool StartupNet();
NET_API bool CleanupNet();
NET_API bool TcpCreate(NetInterface* callback, const std::string& ip, int port, TcpHandle& new_handle);
NET_API bool TcpDestroy(TcpHandle handle);
NET_API bool TcpListen(TcpHandle handle);
NET_API bool TcpConnect(TcpHandle handle, const std::string& ip, int port);
NET_API bool TcpSend(TcpHandle handle, std::unique_ptr<char[]> packet, int size);
NET_API bool TcpGetLocalAddr(TcpHandle handle, char ip[16], int& port);
NET_API bool TcpGetRemoteAddr(TcpHandle handle, char ip[16], int& port);
NET_API bool UdpCreate(NetInterface* callback, const std::string& ip, int port, UdpHandle& new_handle);
NET_API bool UdpDestroy(UdpHandle handle);
NET_API bool UdpSendTo(UdpHandle handle, std::unique_ptr<char[]> packet, int size, const std::string& ip, int port);

} // namespace net

#endif	// NET_INTERFACE_H_