#include "net.h"
#include "res_manager.h"

namespace net {

NET_API bool StartupNet() {
  return SingleResManager::GetInstance()->StartupNet();
}
NET_API bool CleanupNet() {
  if (!SingleResManager::GetInstance()->CleanupNet()) {
    return false;
  }
  SingleResManager::Release();
  return true;
}
NET_API bool TcpCreate(NetInterface* callback, const std::string& ip, int port, TcpHandle& new_handle) {
  return SingleResManager::GetInstance()->TcpCreate(callback, ip, port, new_handle);
}
NET_API bool TcpDestroy(TcpHandle handle) {
  return SingleResManager::GetInstance()->TcpDestroy(handle);
}
NET_API bool TcpListen(TcpHandle handle) {
  return SingleResManager::GetInstance()->TcpListen(handle);
}
NET_API bool TcpConnect(TcpHandle handle, const std::string& ip, int port) {
  return SingleResManager::GetInstance()->TcpConnect(handle, ip, port);
}
NET_API bool TcpSend(TcpHandle handle, std::unique_ptr<char[]> packet, int size) {
  return SingleResManager::GetInstance()->TcpSend(handle, std::move(packet), size);
}
NET_API bool TcpGetLocalAddr(TcpHandle handle, char ip[16], int& port) {
  return SingleResManager::GetInstance()->TcpGetLocalAddr(handle, ip, port);
}
NET_API bool TcpGetRemoteAddr(TcpHandle handle, char ip[16], int& port) {
  return SingleResManager::GetInstance()->TcpGetRemoteAddr(handle, ip, port);
}
NET_API bool UdpCreate(NetInterface* callback, const std::string& ip, int port, UdpHandle& new_handle) {
  return SingleResManager::GetInstance()->UdpCreate(callback, ip, port, new_handle);
}
NET_API bool UdpDestroy(UdpHandle handle) {
  return SingleResManager::GetInstance()->UdpDestroy(handle);
}
NET_API bool UdpSendTo(UdpHandle handle, std::unique_ptr<char[]> packet, int size, const std::string& ip, int port) {
  return SingleResManager::GetInstance()->UdpSendTo(handle, std::move(packet), size, ip, port);
}

} // namespace net