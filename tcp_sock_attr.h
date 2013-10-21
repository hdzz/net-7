// TCP�����ص�һЩ���ԣ�������Щ���Լ���Աע��
// ���̣߳�֧��
// Author: cauloda
// Date: 2013-3-18

#ifndef NET_TCPSOCKATTR_H_
#define NET_TCPSOCKATTR_H_

#include <map>
#include <string>

class TcpSocketAttr {
 public:
	TcpSocketAttr();
	~TcpSocketAttr();
	void SetBeginTime();
	void SetLocalAddr(string ip, int port);
	void SetRemoteAddr(string ip, int port);
	void GetLocalAddr(string& ip, int& port);
	void GetRemoteAddr(string& ip, int& port);
	void AddSendByte(int value);
	void AddRecvByte(int value);
	int GetSendSpeed();
	int GetRecvSpeed();
	int AddPacketCount();

 private:
	TcpSocketAttr(const TcpSocketAttr&);	// ���ɿ��������ɸ�ֵ
	TcpSocketAttr& operator=(const TcpSocketAttr&);

 private:
	long long int send_byte_;	// ������ͳ�ƣ���λΪbytes�������ϲ��ȡһ�η��������߷����ٶȺ��������¿�ʼͳ��
	long long int recv_byte_;	// ͬ��
	DWORD send_time_;					// ��ʼ���㷢������ʱ�䣬���ϲ��ȡһ�η��������߷����ٶȺ����¸�ֵ����ǰʱ�䣩
	DWORD recv_time_;					// ͬ��
	string local_ip_;					// ����ip
	string remote_ip_;				// Զ��ip
	int local_port_;					// ���ض˿�
	int remote_port_;					// Զ�˶˿�
	int packet_count_;				// ���Ͱ�������ȥ����������Ϊ��ֵֻ��Ϊǰ����ı�ʶ
	CriticalSection lock_;		// ͨ������������ʱ�����������ƣ���������

public:
	volatile int last_recv_pkt_;		// �ϴν��յ��İ����б�ʶ
	volatile int last_recv_buff_;		// �ϴ�δ������Ļ����ڰ��е�ƫ�ƣ��������ĸ����壩������δ���������Ϊ-1
	map<int, TcpRecvingPkt*> recving_pkt_;
	CriticalSection recv_lock_;

	TcpRecvingPkt* GetRecvingPkt(int sequence, int total_buff) {
		auto find_pkt = recving_pkt_.find(sequence);
		if (find_pkt == recving_pkt_.end()) {// ��ʼһ���°�
			recving_pkt_.insert(make_pair(sequence, new TcpRecvingPkt(total_buff)));
			return recving_pkt_[sequence];
		} else {// ���ڽ��յ�ĳ��������ɲ���
			return find_pkt->second;
		}
	}
};

#endif	// NET_TCPSOCKATTR_H_