#ifndef NET_UDP_SOCKET_H_
#define NET_UDP_SOCKET_H_

#include <string>
#include <WinSock2.h>

namespace net {

class NetInterface;

class UdpSocket {
 public:
  UdpSocket();
  ~UdpSocket();

  bool Create(NetInterface* callback);
  bool Bind(const std::string& ip, int port);
  void Destroy();
  bool AsyncSendTo(const char* buffer, int size, const std::string& ip, int port, LPOVERLAPPED ovlp);
  bool AsyncRecvFrom(char* buffer, int size, LPOVERLAPPED ovlp, PSOCKADDR_IN addr, PINT addr_size);

  SOCKET socket() { return socket_; }
  NetInterface* callback() { return callback_; }

 private:
  UdpSocket(const UdpSocket&) = delete;
  UdpSocket& operator=(const UdpSocket&) = delete;

 private:
  NetInterface* callback_;
  SOCKET socket_;
  bool bind_;
};

} // namespace net

#endif	// NET_UDP_SOCKET_H_