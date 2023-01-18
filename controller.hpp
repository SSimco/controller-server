#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "controller_data.hpp"

#include <array>
#include <list>
#include <string>

class controller {
protected:
  std::string m_controller_name;

public:
  controller(const std::string &controller_name)
      : m_controller_name(controller_name) {}
  virtual void emit_key(button b, button_state value) = 0;
  virtual void emit_axis(axis a, float value) = 0;
  virtual void reset() = 0;
};

#endif
