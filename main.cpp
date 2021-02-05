#include <iostream>
#include "node.h"
#include <unistd.h>
#include "itoa_ljust.h"

int main(int argc, char **argv) {
	node::inst().set_login_data("127.0.0.1", "3416", "", "", "pool");
	node::inst().start();
	sleep(600);
	node::inst().shutdown();
	return 0;
}
