#include <iostream>
#include "node.h"


int main(int argc, char **argv) {
	node::inst().set_login_data("127.0.0.1", "3416", "user", "test", "pool");
	node::inst().do_getblocktemplate();
	node::inst().shutdown();
	return 0;
}
