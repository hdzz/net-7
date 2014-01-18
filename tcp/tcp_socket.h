#ifndef NET_TCP_SOCKET_H_
#define NET_TCP_SOCKET_H_

#include <string>
#include <vector>
#include <WinSock2.h>

namespace net {

class NetInterface;
class TcpHeader;

class TcpSocket {
 public:
  struct RecvPacket {
    char* packet;
    int size;
    bool need_clear;
    ~RecvPacket() {
      if (need_clear)
        delete[] packet;
    }
  };

  TcpSocket();
  ~TcpSocket();

  bool Create(NetInterface* callback);
  void Destroy();
  bool Bind(const std::string& ip, int port);
  bool Listen(int backlog);
  bool Connect(const std::string& ip, int port);
  bool AsyncAccept(SOCKET accept_sock, char* buffer, int size, LPOVERLAPPED ovlp);
  bool AsyncSend(const TcpHeader* header, const char* buffer, int size, LPOVERLAPPED ovlp);
  bool AsyncRecv(char* buffer, int size, LPOVERLAPPED ovlp);
  bool SetAccepted(SOCKET listen_sock);
  bool GetLocalAddr(std::string& ip, int& port);
  bool GetRemoteAddr(std::string& ip, int& port);

  SOCKET socket() { return socket_; }
  NetInterface* callback() { return callback_; }
  const std::vector<RecvPacket>& all_packets() { return all_packets_; }
  bool OnRecv(const char* data, int size);
  void OnRecvDone() { all_packets_.clear(); }

 private:
  TcpSocket(const TcpSocket&) = delete;
  TcpSocket& operator=(const TcpSocket&) = delete;
  void ResetMember();
  bool CalcPacketSize();
  bool ParseTcpHeader(const char* data, int size, int& parsed_size);
  int ParseTcpPacket(const char* data, int size);

 private:
  NetInterface* callback_;
  SOCKET socket_;
  bool bind_;
  bool listen_;
  bool connect_;
  std::vector<char> current_header_;
  RecvPacket current_packet_;
  int current_packet_offset_;
  std::vector<RecvPacket> all_packets_;
};

} // namespace net

#endif	// NET_TCP_SOCKET_H_