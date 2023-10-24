#pragma once

#define MAX_EVENTS 1024
#define TIMEOUT_S  2

// from nferre/webserv
static inline std::string timestamp() {
	time_t rawtime;
	time(&rawtime);

	tm * timeinfo = localtime(&rawtime);

	char buffer[128];
	strftime(buffer, sizeof(buffer), "[ %T ] ", timeinfo);
	return std::string(buffer);
}

#include <sstream>
#define SSTR(x) static_cast<std::ostringstream const &>((std::ostringstream() << std::dec << x)).str()
