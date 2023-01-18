#ifndef UINPUT_CONTROLLER_HPP
#define UINPUT_CONTROLLER_HPP

#include "controller.hpp"

#include <list>
#include <optional>
#include <string>

class uinput_controller : public controller {
private:
  std::optional<int> m_fd;
  std::list<axis> m_axes;
  std::list<button> m_buttons;
  void emit_ev(unsigned short type, unsigned short code, int value);

public:
  uinput_controller(const std::string &controller_name,
                    const std::list<axis> &axes,
                    const std::list<button> &buttons = {});
  void emit_key(button b, button_state value) override;
  void emit_axis(axis a, float value) override;
  void reset() override;
  ~uinput_controller();
};

#endif
