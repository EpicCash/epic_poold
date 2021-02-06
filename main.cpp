#include <iostream>
#include "node.h"
#include "jconf.hpp"

int main(int argc, char **argv)
{
	const char* conf_filename = "config.json";
	if(argc >= 2)
		conf_filename = argv[1];
	if(!jconf::inst().parse_config(conf_filename))
		return 1;

	node::inst().start();
	sleep(600);
	node::inst().shutdown();
	return 0;
}
