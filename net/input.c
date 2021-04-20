#include "ns.h"
// #include ""

extern union Nsipc nsipcbuf;
#define MAX_BUF_SIZE 2048 - 1

 void
sleep(int msec)
{
	unsigned now = sys_time_msec();
	unsigned end = now + msec;

	if ((int)now < 0 && (int)now > -MAXERROR)
			panic("sys_time_msec: %e", (int)now);

	while (sys_time_msec() < end)
			sys_yield();
}


void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server

	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

	uint8_t buffer[MAX_BUF_SIZE];
	while(1) {
		// poll the driver
		int ret = sys_try_recv_packet(buffer, MAX_BUF_SIZE);
		if(ret == -1) { // empty input queue
			// panic("error occur %d", ret);
			sys_yield();	
		} else if (ret == -2) { // error occurs
			panic("error occur");
		} else if (ret >= 0) {
			cprintf("read packet with length %d \n", ret);
			nsipcbuf.pkt.jp_len = ret;
			memcpy(nsipcbuf.pkt.jp_data, buffer, ret);
			ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_U | PTE_P);
			// sys_yield();
			sleep(50);
		}
	}

}
