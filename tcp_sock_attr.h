// TCP插口相关的一些属性，具体哪些属性见成员注释
// 多线程：支持
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
	TcpSocketAttr(const TcpSocketAttr&);	// 不可拷贝，不可赋值
	TcpSocketAttr& operator=(const TcpSocketAttr&);

 private:
	long long int send_byte_;	// 发送量统计（单位为bytes），在上层获取一次发送量或者发送速度后清零重新开始统计
	long long int recv_byte_;	// 同上
	DWORD send_time_;					// 开始计算发送量的时间，在上层获取一次发送量或者发送速度后重新赋值（当前时间）
	DWORD recv_time_;					// 同上
	string local_ip_;					// 本地ip
	string remote_ip_;				// 远端ip
	int local_port_;					// 本地端口
	int remote_port_;					// 远端端口
	int packet_count_;				// 发送包数，不去理会溢出，因为此值只作为前后包的标识
	CriticalSection lock_;		// 通用锁，但接收时不被次锁限制，另有他锁

public:
	volatile int last_recv_pkt_;		// 上次接收到的包序列标识
	volatile int last_recv_buff_;		// 上次未接收完的缓冲在包中的偏移（即包中哪个缓冲），若无未接收完的则为-1
	map<int, TcpRecvingPkt*> recving_pkt_;
	CriticalSection recv_lock_;

	TcpRecvingPkt* GetRecvingPkt(int sequence, int total_buff) {
		auto find_pkt = recving_pkt_.find(sequence);
		if (find_pkt == recving_pkt_.end()) {// 开始一个新包
			recving_pkt_.insert(make_pair(sequence, new TcpRecvingPkt(total_buff)));
			return recving_pkt_[sequence];
		} else {// 正在接收的某个包的组成部分
			return find_pkt->second;
		}
	}
};

#endif	// NET_TCPSOCKATTR_H_