#include "ns.h"
#include "inc/lib.h"

extern union Nsipc nsipcbuf;
union Nsipc *nsipc = (union Nsipc *)REQVA;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

	uint32_t req, whom;
	int perm, r;
	void *pg;
	bool debug = true;

	while (1) {
		perm = 0;
		req = ipc_recv((int32_t *) &whom, nsipc, &perm); // req 是通过 ipc_send(val) 传递过来的
		if (debug)
			cprintf("fs req %d from %08x [page %08x: %s]\n",
				req, whom, uvpt[PGNUM(nsipc)], nsipc);

		// All requests must contain an argument page
		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n",
				whom);
			continue; // just leave it hanging...
		}
		uint32_t n = 0;
		while (1) {
			n = sys_try_send_packet(nsipc->pkt.jp_data, nsipc->pkt.jp_len);
			if (n == nsipc->pkt.jp_len) {
				break;
			} else {
				panic("nic unavaiable %d", n);
			}
		}
		sys_page_unmap(0, nsipc);
	}	
}
