#ifndef UDP_CLIENT_HPP
#define UDP_CLIENT_HPP

#include <boost/array.hpp>
#include <boost/asio.hpp>

const size_t udp_buffer_size = 1024;

class udp_client {
  boost::asio::io_service io_service;
  boost::asio::ip::udp::socket socket;
  boost::asio::ip::udp::endpoint sender;
  boost::array<char, udp_buffer_size> recieve_buffer;
  std::function<void(const boost::array<char, udp_buffer_size>&)> m_on_msg;

 public:
  void recieve();

  udp_client(unsigned short udp_port,
             std::function<void(const boost::array<char, udp_buffer_size>&)>
                 on_msg = {});
  ~udp_client() { io_service.stop(); }
};

#endif
