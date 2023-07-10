#include <iostream>
#include <fstream>

#include "WebservConfig.hpp"

int main(int argc, char** argv) {

	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
		return 1;
	}

	WebservConfig config(argv[1]);

	return 0;
}

// Authors : Charly Tardy, Marwan Ayoub, Noah Alexandre
// Version : 0.2.1
