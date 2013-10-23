#ifndef NET_TCP_SOCK_ATTR_H_
#define NET_TCP_SOCK_ATTR_H_

#include <mutex>
#include <string>

namespace net {

class TcpSocketAttr {
 public:
	TcpSocketAttr();
	~TcpSocketAttr();
	void SetBeginTime();
	void SetLocalAddr(std::string ip, int port);
	void SetRemoteAddr(std::string ip, int port);
	void GetLocalAddr(std::string& ip, int& port);
	void GetRemoteAddr(std::string& ip, int& port);
	void AddSendByte(int value);
	void AddRecvByte(int value);
	int GetSendSpeed();
	int GetRecvSpeed();

 private:
	TcpSocketAttr(const TcpSocketAttr&);	// 不可拷贝，不可赋值
	TcpSocketAttr& operator=(const TcpSocketAttr&);

 private:
	long long int send_byte_;	        // 发送量统计（单位为bytes），在上层获取一次发送量或者发送速度后清零重新开始统计
	long long int recv_byte_;	        // 同上
	unsigned long send_begin_time_;	  // 开始计算发送量的时间，在上层获取一次发送量或者发送速度后重新赋值（当前时间）
	unsigned long recv_begin_time_;	  // 同上
  std::string local_ip_;	          // 本地ip
  std::string remote_ip_;	          // 远端ip
  int local_port_;					        // 本地端口
  int remote_port_;					        // 远端端口
  std::mutex lock_;		              // 通用锁
};

} // namespace net

#endif	// NET_TCP_SOCK_ATTR_H_