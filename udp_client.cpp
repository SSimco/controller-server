#include "udp_client.hpp"

void udp_client::recieve() {
  auto bytes_transferred =
      socket.receive_from(boost::asio::buffer(recieve_buffer), sender);
  if (bytes_transferred) {
    m_on_msg(recieve_buffer);
  }
  recieve();
}

udp_client::udp_client(
    unsigned short udp_port,
    std::function<void(const boost::array<char, udp_buffer_size>&)> on_msg)
    : socket{io_service, {{}, udp_port}}, m_on_msg(on_msg) {
  io_service.run();
}
