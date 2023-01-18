#include "uinput_controller.hpp"
#include "controller.hpp"

#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>
#include <string>
#include <type_traits>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <stdexcept>
#include <tuple>

const int max_abs_axis_value = 32768;

unsigned short button_to_uinput_code(button b) {
  switch (b) {
  case button::A:
    return BTN_A;
  case button::B:
    return BTN_B;
  case button::X:
    return BTN_X;
  case button::Y:
    return BTN_Y;
  case button::TRIGGER_LEFT1:
    return BTN_TL;
  case button::TRIGGER_RIGHT1:
    return BTN_TR;
  case button::TRIGGER_LEFT2:
    return BTN_TL2;
  case button::TRIGGER_RIGHT2:
    return BTN_TL2;
  case button::SELECT:
    return BTN_SELECT;
  case button::START:
    return BTN_START;
  case button::DPAD_UP:
    return BTN_DPAD_UP;
  case button::DPAD_DOWN:
    return BTN_DPAD_DOWN;
  case button::DPAD_LEFT:
    return BTN_DPAD_LEFT;
  case button::DPAD_RIGHT:
    return BTN_DPAD_RIGHT;
  default:
    throw std::runtime_error(
        std::string("unknown button value: ") +
        std::to_string(static_cast<std::underlying_type_t<button>>(b)));
  }
}

unsigned short axis_to_uinput_code(axis a) {
  switch (a) {
  case axis::X:
  case axis::ACCEL_X:
    return ABS_X;
  case axis::Y:
  case axis::ACCEL_Y:
    return ABS_Y;
  case axis::RX:
  case axis::GYRO_X:
    return ABS_RX;
  case axis::RY:
  case axis::GYRO_Y:
    return ABS_RY;
  case axis::ACCEL_Z:
    return ABS_Z;
  case axis::GYRO_Z:
    return ABS_RZ;
  default:
    throw std::runtime_error(
        std::string("unknown axis value: ") +
        std::to_string(static_cast<std::underlying_type_t<axis>>(a)));
  }
}

class error_from_errnum : public std::runtime_error {
public:
  error_from_errnum(const std::string &msg, int errnum)
      : std::runtime_error(msg + std::string(strerror(errnum))) {}
};

class ioctl_error : public error_from_errnum {
public:
  ioctl_error(int errnum) : error_from_errnum("ioctl error: ", errnum) {}
};
class write_error : public error_from_errnum {
public:
  write_error(int errnum) : error_from_errnum("write error: ", errnum) {}
};
class file_descriptor_error : public std::runtime_error {
public:
  file_descriptor_error()
      : std::runtime_error("file descriptor was not initialized") {}
};

template <typename... TArgs>
void _ioctl(const std::optional<int> &fd, TArgs... args) {
  if (!fd.has_value()) {
    throw file_descriptor_error();
  }
  if (int errnum = ioctl(fd.value(), std::forward<TArgs>(args)...)) {
    throw ioctl_error(errnum);
  }
}

uinput_controller::uinput_controller(const std::string &name,
                                     const std::list<axis> &axes,
                                     const std::list<button> &buttons)
    : controller(name), m_buttons(buttons), m_axes(axes) {
  m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

  if (!buttons.empty()) {
    _ioctl(m_fd, UI_SET_EVBIT, EV_KEY);
  }
  if (!axes.empty()) {
    _ioctl(m_fd, UI_SET_EVBIT, EV_ABS);
  }
  if (buttons.empty() && !axes.empty()) {
    _ioctl(m_fd, UI_SET_PROPBIT, INPUT_PROP_ACCELEROMETER);
  }
  for (auto &&button : buttons) {
    _ioctl(m_fd, UI_SET_KEYBIT, button_to_uinput_code(button));
  }
  for (axis code : axes) {
    auto mapped_code = axis_to_uinput_code(code);
    _ioctl(m_fd, UI_SET_ABSBIT, mapped_code);

    uinput_abs_setup abs_setup = {
        .code = mapped_code,
        .absinfo = {.minimum = -max_abs_axis_value,
                    .maximum = max_abs_axis_value},
    };
    _ioctl(m_fd, UI_ABS_SETUP, &abs_setup);
  }
  uinput_setup setup = {.id = {
                            .bustype = BUS_VIRTUAL,
                            .vendor = 0x0123,
                            .product = 0x0321,
                        }};
  std::copy_n(name.cbegin(),
              std::min(UINPUT_MAX_NAME_SIZE, static_cast<int>(name.size())),
              setup.name);
  _ioctl(m_fd, UI_DEV_SETUP, &setup);
  _ioctl(m_fd, UI_DEV_CREATE);
}

void uinput_controller::emit_ev(unsigned short type, unsigned short code,
                                int value) {
  if (!m_fd.has_value()) {
    throw file_descriptor_error();
  }

  input_event ev[] = {{.type = type, .code = code, .value = value},
                      {.type = EV_SYN, .code = SYN_REPORT, .value = 0}};

  if (int errnum = write(m_fd.value(), &ev, sizeof(ev)); errnum < 0) {
    throw write_error(errnum);
  }
}

void uinput_controller::emit_axis(axis a, float value) {
  int mapped_value = value * max_abs_axis_value;
  emit_ev(EV_ABS, axis_to_uinput_code(a), mapped_value);
}

void uinput_controller::emit_key(button b, button_state value) {
  emit_ev(EV_KEY, button_to_uinput_code(b), static_cast<int>(value));
}

void uinput_controller::reset() {
  for (auto axis : m_axes) {
    emit_axis(axis, 0);
  }
  for (auto button : m_buttons) {
    emit_key(button, button_state::OFF);
  }
}

uinput_controller::~uinput_controller() {
  if (m_fd.has_value()) {
    close(m_fd.value());
  }
}
