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

#if 0
	char prepow[] = "000600000000000ce32200000000603be64ed3e8a9ce3df01690f0b024f61052faf75e08c10babd649440bb678dd70e3199a957152836ab1349ac88a6f5930c3571f66fb25b6cc4b5cd1e1cf7090a9d89a5cb50bdadc854a50669bd2fa077cfa965bc49956bf69c30549543b073caa4ab0e3738b6fb5240c03d0d67adc3ca83ad74564afd0d08449a4f96144ae3e71bf1c21032bd4c56d6dede037bf7fa075a59af4b038e3d7cbacb5474ef24e4c74688762a5f96381e7f47a3981c39154193429606251513b9ea379315f99647681fa222f00000000002561cf00000000001bc8400000000000000004000000000000338c8c0100000068fa047a59020001ba332b70f43a03071ee1dda24c4ff5fb8a506d";

	char hash[] = "fa77eb913694d0fd402d8dc98f8726f85bf09628783265a38aecbe03ee40fc01";

	v32 ds;
	if(!hex2bin(hash, 64, ds.data))
		return 1;

	hashpool::inst().calculate_dataset(ds);
	int ts = 15;
	while((ts = sleep(ts)) != 0);

	uint8_t ppow[1024];
	int ln = strlen(prepow);
	if(!hex2bin(prepow, ln, ppow))
		return 1;

	ln /= 2;
	uint64_t test = __builtin_bswap64(6070405701865009284ul);
	memcpy(ppow+ln, &test, 8);
	ln += 8;

	std::future<void> future;
	hashpool::check_job job;
	job.data = ppow;
	job.data_len = ln;
	job.dataset_id = ds.get_id();
	future = job.ready.get_future();
	
	hashpool::inst().push_job(job);
	future.wait();
	
	if(job.error)
	{
		return 1;
	}

	char out[65];
	bin2hex(job.hash.data, 32, out);
	printf("%s\n", out);
	return 0;
#endif 

	node::inst().start();
	server::inst().start();

	while(true)
		sleep(600);
	node::inst().shutdown();
	return 0;
}
