#include <iostream>
#include "node.h"
#include "server.hpp"
#include "jconf.hpp"
#include "hashpool.hpp"

int main(int argc, char **argv)
{
	const char* conf_filename = "config.json";
	if(argc >= 2)
		conf_filename = argv[1];
	if(!jconf::inst().parse_config(conf_filename))
		return 1;

	node::inst().start();
	server::inst().start();

	while(true)
		sleep(600);
	node::inst().shutdown();
	return 0;
}
