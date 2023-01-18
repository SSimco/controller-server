#include <functional>
#include <linux/input-event-codes.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "controller.hpp"
#include "udp_client.hpp"
#include "uinput_controller.hpp"
#include <boost/signals2.hpp>

template <typename T, size_t array_size, size_t buffer_size>
void copy_from_buffer(std::array<T, array_size> &array,
                      const boost::array<char, buffer_size> &buffer,
                      size_t &index) {
  const T *data;
  data = reinterpret_cast<const T *>(buffer.data() + index);
  std::copy(data, data + array_size, array.begin());
  index += array_size * sizeof(T);
}
template <typename T, size_t buffer_size>
T read_from_buffer(const boost::array<char, buffer_size> &buffer,
                   size_t &index) {
  T val = *reinterpret_cast<const T *>(buffer.data() + index);
  index += sizeof(T);
  return val;
}
int main(int argc, char *argv[]) {
  boost::signals2::signal<void(button, button_state)> key_signal;
  boost::signals2::signal<void(axis, float)> axis_signal;
  boost::signals2::signal<void(axis, float)> gyro_signal;

  uinput_controller main_controller(
      "Controller", {axis::X, axis::Y, axis::RX, axis::RY},
      {button::A, button::B, button::X, button::Y, button::TRIGGER_LEFT1,
       button::TRIGGER_RIGHT1, button::TRIGGER_LEFT2, button::TRIGGER_RIGHT2,
       button::SELECT, button::START, button::DPAD_UP, button::DPAD_DOWN,
       button::DPAD_LEFT, button::DPAD_RIGHT});
  key_signal.connect(std::bind(&controller::emit_key, &main_controller,
                               std::placeholders::_1, std::placeholders::_2));
  axis_signal.connect(std::bind(&controller::emit_axis, &main_controller,
                                std::placeholders::_1, std::placeholders::_2));
  auto on_msg = [&](const boost::array<char, udp_buffer_size> &buffer) {
    size_t index = 0;
    input_type in_type = read_from_buffer<input_type>(buffer, index);
    switch (in_type) {
    case input_type::key: {
      button key = read_from_buffer<button>(buffer, index);
      button_state val = read_from_buffer<button_state>(buffer, index);
      key_signal(key, val);
      break;
    }
    case input_type::axis: {
      axis axis_code = read_from_buffer<axis>(buffer, index);
      double val = read_from_buffer<double>(buffer, index);
      axis_signal(axis_code, val);
      break;
    }
    case input_type::gyro: {
      static const std::vector gyro_axes = {axis::GYRO_X, axis::GYRO_Y,
                                            axis::GYRO_Z};
      static const std::vector accel_axes = {axis::ACCEL_X, axis::ACCEL_Y,
                                             axis::ACCEL_Z};
      std::array<int, 3> gyro_arr, accel_arr;
      copy_from_buffer(gyro_arr, buffer, index);
      copy_from_buffer(accel_arr, buffer, index);
      for (size_t i = 0; i < 3; i++) {
        gyro_signal(gyro_axes[i], gyro_arr[i]);
        gyro_signal(accel_axes[i], accel_arr[i]);
      }
      break;
    }
    default:
      std::cerr << "Unkown input type: " << static_cast<int>(in_type)
                << std::endl;
    }
  };
  const unsigned short defaultPort = 15366;
  unsigned short port;
  if (argc > 1) {
    try {
      port = std::stoi(argv[1]);
    } catch (std::invalid_argument inv_arg) {
      std::cerr << "Bad port: " << argv[1] << std::endl;
      port = defaultPort;
    }
  } else {
    port = defaultPort;
  }
  udp_client client(port, on_msg);
  client.recieve();
}
