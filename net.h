#ifndef NET_INTERFACE_H_
#define NET_INTERFACE_H_

namespace net {

typedef unsigned long TcpLink;			// 自定义TCP\UDP链接类型，对用户透明，防止误操作
typedef unsigned long UdpLink;

const TcpLink kInvalidTcpLink = ~0;	// 定义无效链接
const UdpLink kInvalidUdpLink = ~0;

const int kMaxTcpPacketSize = 16 * 1024 * 1024;	// 定义TCP\UDP包最大值
const int kMaxUdpPacketSize = 8 * 1024;

enum class StartupType { kStartupTcp, kStartupUdp, kStartupBoth };	// 需要启动的协议
enum class EitherPeer { kLocal, kRemote };						// 需要获取本地还是远端的TCP链接地址
enum class AttributeType { kSendSpeed, kRecvSpeed };	// 限速、获取速度

// 此类是所有网络服务的基类，想要创建一个网络服务，只需要继承此类，并重写所有纯虚函数即可
class NetInterface {
 public:
	// 当TCP链接被断开时回调
	virtual bool OnTcpLinkDisconnected(TcpLink link) = 0;

	// 当TCP链接在监听状态下，对端发起连接，并成功Accepted时回调
	virtual bool OnTcpLinkAccepted(TcpLink link, TcpLink accept_link) = 0;

	// 当接收到TCP包时回调
	virtual bool OnTcpLinkReceived(TcpLink link, const char* packet, int size) = 0;

	// 当TCP链接出错时回调
	virtual bool OnTcpLinkError(TcpLink link, int error) = 0;

	// 当接收到UDP包时回调
	virtual bool OnUdpLinkReceived(UdpLink link, const char* packet, int size, const char* ip, int port) = 0;

	// 当UDP链接出错时回调
	virtual bool OnUdpLinkError(UdpLink link, int error) = 0;
};

#ifdef NET_EXPORTS
#define NET_API _declspec(dllexport)
#else 
#define NET_API _declspec(dllimport)
#endif // NET_EXPORTS

// 启用网络：输入需要启动的协议，后续工作之前必须先调用这个函数
NET_API bool StartupNet(StartupType type);

// 关闭网络：所有正在进行中的网络操作都将被终止，并且清理所有网络资源
NET_API bool CleanupNet();

// 创建一个TCP链接：输入字符串形式IP地址和端口，输出一个新的TCP链接
NET_API bool CreateTcpLink(NetInterface* callback, const char* ip, int port, TcpLink& new_link);

// 销毁一个TCP链接：清理所有与此链接相关的资源，此端口在函数返回成功后可立刻被再使用
NET_API bool DestroyTcpLink(TcpLink link);

// 监听一个TCP链接：Accept上来的链接与被监听的链接具有相同属性
NET_API bool Listen(TcpLink link);

// 连接到对端：连接并建立一条新的TCP链接
NET_API bool Connect(TcpLink link, const char* ip, int port);

// 发送TCP包：接收端接收到该大小的数据后将进行回调，发送大小限制为16M
NET_API bool SendTcpPacket(TcpLink link, const char* packet, int size);

// 获取TCP链接源地址或者目的地址，输出IP地址和端口
NET_API bool GetTcpLinkAddr(TcpLink link, EitherPeer type, char ip[16], int& port);

// 设置TCP链接的发送或者接收速度（单位bytes/s）
NET_API bool SetTcpLinkAttr(TcpLink link, AttributeType type, int value);

// 获取TCP链接的发送或者接收速度（单位bytes/s）
NET_API bool GetTcpLinkAttr(TcpLink link, AttributeType type, int& value);

// 创建一个UDP链接：输入字符串形式IP地址和端口以及是否需要广播，输出一个新的UDP链接
NET_API bool CreateUdpLink(NetInterface* callback, const char* ip, int port, bool broadcast, UdpLink& new_link);

// 销毁一个UDP链接：清理所有与此链接相关的资源，此端口在函数返回成功后可立刻被再使用
NET_API bool DestroyUdpLink(UdpLink link);

// 发送UDP包：接收端接收到该大小的数据后将进行回调，发送大小限制为8K
NET_API bool SendUdpPacket(UdpLink link, const char* packet, int size, const char* ip, int port);

} // namespace net

#endif	// NET_INTERFACE_H_