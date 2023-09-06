#ifndef EXITCODE_HPP
# define EXITCODE_HPP

enum ExitCode {
	SUCCESS,
	USAGE_FAILURE,
	SIGNAL_FAILURE,
	CLEANUP_FAILURE,
	CONFIG_FAILURE,
	LISTEN_FAILURE,
	LAUNCH_FAILURE,
	CGI_FAILURE
};

#endif