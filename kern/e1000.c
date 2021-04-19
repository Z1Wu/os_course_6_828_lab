#include <kern/e1000.h>
#include <kern/pmap.h>
#include <kern/e1000_hw.h>

volatile void *e1000c;

struct tx_desc tx_decs_queue[TX_DESCRIPTOR_QUEUE_SIZE];
uint8_t tx_buffer[TX_DESCRIPTOR_QUEUE_SIZE * TX_BUF_SIZE]; // need to be continuous

struct e1000_tail* tdt;

// LAB 6: Your driver code here
int e1000_attachfn (struct pci_func *pcif) {
    // enbale pcif just for now
    
    pci_func_enable(pcif);

    physaddr_t nic_pa = pcif->reg_base[0];
    uint32_t nic_size =  pcif->reg_size[0];
    if (!nic_pa || nic_size == 0)
		return 0;
	// lapicaddr is the physical address of the LAPIC's 4K MMIO
	// region.  Map it in to virtual memory so we can access it.
	e1000c = mmio_map_region(nic_pa, nic_size);

    for(int i = 0; i < TX_DESCRIPTOR_QUEUE_SIZE; i++) {
        tx_decs_queue[i].addr_low = PADDR(tx_buffer + i * TX_BUF_SIZE);
        tx_decs_queue[i].addr_high = 0;
        tx_decs_queue[i].length = TX_BUF_SIZE;
        tx_decs_queue[i].dd = 1; // mark as no used
        tx_decs_queue[i].rs = 1; // report status
    }
    // cprintf("descriptor size %d \n", sizeof(tx_decs_queue[0]));
    assert(sizeof(tx_decs_queue[0]) == 16);

    // cprintf("init tdba \n");
    struct e1000_tdba * tdba = (struct e1000_tdba *)(e1000c + E1000_TDBAL);
    tdba->l = PADDR(tx_decs_queue);
    
    // init
    // cprintf("init tdlent \n");
    *(uint32_t *)(e1000c + E1000_TDLEN) = TX_DESCRIPTOR_QUEUE_SIZE * 16; // 16 descritor size
    
    // init tdh and tdt
    // struct e1000_head* tdh = (struct e1000_head*)(e1000c + E1000_TDH);
    tdt = (struct e1000_tail*)(e1000c + E1000_TDT);
    // cprintf("init tdt %d\n", sizeof(*tdt));
    // tdh->p = 0;
    tdt->p = 0;

    // setup tctl register
    struct e1000_tctl* tctl = (struct e1000_tctl* )(e1000c + E1000_TCTL);
    // cprintf("init tctl %d \n", sizeof(*tctl));
    tctl->en = 1;
    tctl->cold = 0x40;

    // cprintf("init tipg \n");
    // setup ipg register
    struct e1000_tipg* tipg = (struct e1000_tipg* )(e1000c + E1000_TIPG);
    tipg->ipgt = 10;
    tipg->ipgr1 = 8;
    tipg->ipgr1 = 6;

    cprintf("register staus %0x \n", *((uint32_t *)(e1000c + E1000_STATUS)));
    
    // char test[] = "abcdtestlhjktheskh";
    // for(int i = 0; i < TX_DESCRIPTOR_QUEUE_SIZE; i++) {
    //     send_one_packet(test, i + 1);
    // }
    // send_one_packet(test, 4);
    // panic("break");
    return 0;
}

// try send one buffer
int send_one_packet(void* packet, uint32_t size) {
    // 返回成功发送的字节数
    // if send sussful else 

    // check whether have free descriptor
    assert(size <= TX_BUF_SIZE);
    if(tx_decs_queue[tdt->p].dd == 1) { // 发送的字节数
        tx_decs_queue[tdt->p].dd = 0;
        memmove((void *)KADDR(tx_decs_queue[tdt->p].addr_low), packet, size);
        tx_decs_queue[tdt->p].length = size;
        tx_decs_queue[tdt->p].eop = 1;
        tdt->p = (tdt->p + 1) % TX_DESCRIPTOR_QUEUE_SIZE;
        return size;
    } else {
        return -1;
    }
}