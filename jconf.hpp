#pragma once
#include <inttypes.h>
#include <stddef.h>
#include "loglevels.hpp"

class jconf
{
public:
	static jconf& inst()
	{
		static jconf inst;
		return inst;
	};

	bool parse_config(const char* filename);

	const char* get_pid_file();
	const char* get_log_file();
	log_level get_log_level();

	const char* get_db_hostname();
	const char* get_db_port();
	const char* get_db_name();
	const char* get_db_username();
	const char* get_db_password();

	const char* get_node_hostname();
	const char* get_node_port();
	const char* get_node_username();
	const char* get_node_password();

	bool daemonize();

	uint16_t get_server_port();
	uint16_t get_server_tls_port();

	const char* get_tls_cert_filename();
	const char* get_tls_cipher_list();

	size_t get_fatal_node_timeout();
	size_t get_template_timeout();

private:
	jconf();
	class jconfPrivate& d;
};
