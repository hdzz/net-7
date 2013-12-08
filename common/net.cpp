#include "net.h"
#include "res_manager.h"
#include "singleton.h"

namespace net {

typedef utility::Singleton<ResManager> SingleResMgr;

// ���º���ֻ�ǽ��ӿں�����ʵ��ת������Դ�������н���
NET_API bool StartupNet() {
	return SingleResMgr::GetInstance()->StartupNet();
}
NET_API bool CleanupNet() {
	if (!SingleResMgr::GetInstance()->CleanupNet()) {
		return false;
	}
	SingleResMgr::Release();
	return true;
}
NET_API bool CreateTcpLink(NetInterface* callback, const char* ip, int port, TcpLink& new_link) {
	return SingleResMgr::GetInstance()->CreateTcpLink(callback, ip, port, new_link);
}
NET_API bool DestroyTcpLink(TcpLink link) {
	return SingleResMgr::GetInstance()->DestroyTcpLink(link);
}
NET_API bool Listen(TcpLink link, int backlog) {
	return SingleResMgr::GetInstance()->Listen(link, backlog);
}
NET_API bool Connect(TcpLink link, const char* ip, int port) {
	return SingleResMgr::GetInstance()->Connect(link, ip, port);
}
NET_API bool SendTcpPacket(TcpLink link, const char* packet, int size) {
	return SingleResMgr::GetInstance()->SendTcpPacket(link, packet, size);
}
NET_API bool GetTcpLinkLocalAddr(TcpLink link, char ip[16], int& port) {
	return SingleResMgr::GetInstance()->GetTcpLinkLocalAddr(link, ip, port);
}
NET_API bool GetTcpLinkRemoteAddr(TcpLink link, char ip[16], int& port) {
	return SingleResMgr::GetInstance()->GetTcpLinkRemoteAddr(link, ip, port);
}
NET_API bool CreateUdpLink(NetInterface* callback, const char* ip, int port, bool broadcast, UdpLink& new_link) {
	return SingleResMgr::GetInstance()->CreateUdpLink(callback, ip, port, broadcast, new_link);
}
NET_API bool DestroyUdpLink(UdpLink link) {
	return SingleResMgr::GetInstance()->DestroyUdpLink(link);
}
NET_API bool SendUdpPacket(UdpLink link, const char* packet, int size, const char* ip, int port) {
	return SingleResMgr::GetInstance()->SendUdpPacket(link, packet, size, ip, port);
}

} // namespace net