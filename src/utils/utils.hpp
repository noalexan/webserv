#pragma once

#define MAX_EVENTS 1024
#define TIMEOUT_S  2

#include <sstream>
#define SSTR(x) static_cast<std::ostringstream const &>((std::ostringstream() << std::dec << x)).str()
