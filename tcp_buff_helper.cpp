#include "tcp_buff_helper.h"

namespace net {

const int kTcpHeadSize = sizeof(TcpHead);		// TCP包头大小
const int kTrueFirstBuffSize = kTcpBufferSize - kTcpHeadSize; // TCP首个缓冲实际可用大小

// 根据上层包大小计算需要多少个发送缓冲
int CalcSendBuffNum(int packet_size) {
  if (packet_size <= kTrueFirstBuffSize) {
    return 1;
  }
  auto buffer_num = 1;
  auto left_size = packet_size - kTrueFirstBuffSize;
  buffer_num += left_size / kTcpBufferSize;
  if (left_size % kTcpBufferSize > 0) {
    ++buffer_num;
  }
  return buffer_num;
}

// 填充所有发送缓冲
void FillTcpSendBuff(const char* packet, int size, std::vector<TcpSendBuff*>& buffer) {
  //auto buffer_num = buffer.size();
  //if (buffer_num <= 1) {

  //} else {

  //}


  //auto fill_offset = packet;
  //auto fill_left = size;
  //auto buffer_num = buffer.size();
  //for (auto i=0; i<buffer_num; ++i) {
  //  auto& each_buffer = buffer.at(i);
  //  auto each_
  //  if (i == 0) {
  //    TcpHead head;
  //    head.packet_size_ = size;
  //    head.checksum_ = 0;
  //    each_buffer->set_buffer((char*)&head, 0, kTcpHeadSize);
  //  }


    //char* copy_head = const_cast<char*>(buffer[i]->buffer());
    //char* copy_data = copy_head + kTcpBuffHeadSize;
    //if (i != buffer_num - 1) {// 未到最后一个缓冲，应用数据填满整个缓冲
    //  head.data_size_ = kMaxTrueSRBuffSize;
    //  memcpy(copy_head, &head, kTcpBuffHeadSize);
    //  memcpy(copy_data, each_buffer, kMaxTrueSRBuffSize);
    //  buffer[i]->set_buffer_size(kTcpBufferSize);
    //  each_buffer += kMaxTrueSRBuffSize;
    //  size_left -= kMaxTrueSRBuffSize;
    //} else {// 到最后一个缓冲了，应该将其8字节对齐
    //  int tail_size = size_left % kTcpBuffHeadSize;
    //  int fill_tail = 0;
    //  if (tail_size != 0) {
    //    fill_tail = kTcpBuffHeadSize - tail_size;
    //  }
    //  int last_copy_size = size_left + fill_tail;
    //  int last_buffer_size = last_copy_size + kTcpBuffHeadSize;
    //  head.data_size_ = size_left;
    //  memcpy(copy_head, &head, kTcpBuffHeadSize);
    //  memcpy(copy_data, each_buffer, last_copy_size);
    //  buffer[i]->set_buffer_size(last_buffer_size);
    //}
  //}
}

} // namespace net