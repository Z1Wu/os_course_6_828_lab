#include <kern/e1000.h>
#include <kern/pmap.h>
#include <kern/e1000_hw.h>

volatile void *e1000c;

struct tx_desc tx_decs_queue[TX_DESCRIPTOR_QUEUE_SIZE];
uint8_t tx_buffer[TX_DESCRIPTOR_QUEUE_SIZE * TX_BUF_SIZE]; // need to be continuous


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
        tx_decs_queue[i].addr = PADDR(tx_buffer + i * TX_BUF_SIZE);
        tx_decs_queue[i].length = TX_BUF_SIZE;
        tx_decs_queue[i].status.dd = 0;
    }

    // cprintf("init tdba \n");
    struct e1000_tdba * tdba = (struct e1000_tdba *)(e1000c + E1000_TDBAL);
    tdba->l = PADDR(tx_decs_queue);
    
    // init
    // cprintf("init tdlent \n");
    *(uint32_t *)(e1000c + E1000_TDLEN) = TX_DESCRIPTOR_QUEUE_SIZE * 16; // 16 descritor size
    
    // init tdh and tdt
    // struct e1000_head* tdh = (struct e1000_head*)(e1000c + E1000_TDH);
    struct e1000_tail* tdt = (struct e1000_tail*)(e1000c + E1000_TDT);
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
    
    return 0;
}



