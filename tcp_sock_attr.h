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
	TcpSocketAttr(const TcpSocketAttr&);	// ���ɿ��������ɸ�ֵ
	TcpSocketAttr& operator=(const TcpSocketAttr&);

 private:
	long long int send_byte_;	        // ������ͳ�ƣ���λΪbytes�������ϲ��ȡһ�η��������߷����ٶȺ��������¿�ʼͳ��
	long long int recv_byte_;	        // ͬ��
	unsigned long send_begin_time_;	  // ��ʼ���㷢������ʱ�䣬���ϲ��ȡһ�η��������߷����ٶȺ����¸�ֵ����ǰʱ�䣩
	unsigned long recv_begin_time_;	  // ͬ��
  std::string local_ip_;	          // ����ip
  std::string remote_ip_;	          // Զ��ip
  int local_port_;					        // ���ض˿�
  int remote_port_;					        // Զ�˶˿�
  std::mutex lock_;		              // ͨ����
};

} // namespace net

#endif	// NET_TCP_SOCK_ATTR_H_