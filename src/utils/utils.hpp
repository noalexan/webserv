#pragma once

#include <sstream>
#define SSTR(x) static_cast<std::ostringstream const &>((std::ostringstream() << std::dec << x)).str()
