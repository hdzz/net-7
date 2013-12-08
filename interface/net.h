#ifndef NET_INTERFACE_H_
#define NET_INTERFACE_H_

namespace net {

typedef unsigned long TcpLink;	// �Զ���TCP\UDP�������ͣ����û�͸������ֹ�����
typedef unsigned long UdpLink;

const TcpLink kInvalidTcpLink = 0;	// ������Ч����
const UdpLink kInvalidUdpLink = 0;

const int kMaxListenBacklogSize = 64;	// ͬʱ���տͻ���������
const int kMaxTcpPacketSize = 16 * 1024 * 1024;	// ����TCP\UDP�����ֵ
const int kMaxUdpPacketSize = 8 * 1024;

// �����������������Ļ��࣬��Ҫ����һ���������ֻ��Ҫ�̳д��࣬����д���д��麯������
class NetInterface {
 public:
	// ��TCP���ӱ��Ͽ�ʱ�ص�
	virtual bool OnTcpLinkDisconnected(TcpLink link) = 0;

	// ��TCP�����ڼ���״̬�£��Զ˷������ӣ����ɹ�Acceptedʱ�ص�
	virtual bool OnTcpLinkAccepted(TcpLink link, TcpLink accept_link) = 0;

	// �����յ�TCP��ʱ�ص�
	virtual bool OnTcpLinkReceived(TcpLink link, const char* packet, int size) = 0;

	// ��TCP���ӳ���ʱ�ص�
	virtual bool OnTcpLinkError(TcpLink link, int error) = 0;

	// �����յ�UDP��ʱ�ص�
	virtual bool OnUdpLinkReceived(UdpLink link, const char* packet, int size, const char* ip, int port) = 0;

	// ��UDP���ӳ���ʱ�ص�
	virtual bool OnUdpLinkError(UdpLink link, int error) = 0;
};

#ifdef NET_EXPORTS
#define NET_API _declspec(dllexport)
#else 
#define NET_API _declspec(dllimport)
#endif // NET_EXPORTS

// �������磺������Ҫ������Э�飬��������֮ǰ�����ȵ����������
NET_API bool StartupNet();

// �ر����磺�������ڽ����е����������������ֹ��������������������Դ
NET_API bool CleanupNet();

// ����һ��TCP���ӣ������ַ�����ʽIP��ַ�Ͷ˿ڣ����һ���µ�TCP����
NET_API bool CreateTcpLink(NetInterface* callback, const char* ip, int port, TcpLink& new_link);

// ����һ��TCP���ӣ������������������ص���Դ���˶˿��ں������سɹ�������̱���ʹ��
NET_API bool DestroyTcpLink(TcpLink link);

// ����һ��TCP���ӣ�Accept�����������뱻���������Ӿ�����ͬ����
NET_API bool Listen(TcpLink link, int backlog);

// ���ӵ��Զˣ����Ӳ�����һ���µ�TCP����
NET_API bool Connect(TcpLink link, const char* ip, int port);

// ����TCP�������ն˽��յ��ô�С�����ݺ󽫽��лص������ʹ�С����Ϊ16M
NET_API bool SendTcpPacket(TcpLink link, const char* packet, int size);

// ��ȡTCP����Դ��ַ�����IP��ַ�Ͷ˿�
NET_API bool GetTcpLinkLocalAddr(TcpLink link, char ip[16], int& port);

// ��ȡTCP����Ŀ�ĵ�ַ�����IP��ַ�Ͷ˿�
NET_API bool GetTcpLinkRemoteAddr(TcpLink link, char ip[16], int& port);

// ����һ��UDP���ӣ������ַ�����ʽIP��ַ�Ͷ˿��Լ��Ƿ���Ҫ�㲥�����һ���µ�UDP����
NET_API bool CreateUdpLink(NetInterface* callback, const char* ip, int port, bool broadcast, UdpLink& new_link);

// ����һ��UDP���ӣ������������������ص���Դ���˶˿��ں������سɹ�������̱���ʹ��
NET_API bool DestroyUdpLink(UdpLink link);

// ����UDP�������ն˽��յ��ô�С�����ݺ󽫽��лص������ʹ�С����Ϊ8K
NET_API bool SendUdpPacket(UdpLink link, const char* packet, int size, const char* ip, int port);

} // namespace net

#endif	// NET_INTERFACE_H_