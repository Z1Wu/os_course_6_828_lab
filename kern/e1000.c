#include <kern/e1000.h>
#include <kern/pmap.h>
#include <kern/e1000_hw.h>

volatile void *e1000c;

struct tx_desc tx_decs_queue[TX_DESCRIPTOR_QUEUE_SIZE]; // 16 byte 对齐
struct rec_desc rec_decs_queue[REC_DESCRIPTOR_QUEUE_SIZE]; // 16 byte 对齐

uint8_t tx_buffer[TX_DESCRIPTOR_QUEUE_SIZE * TX_BUF_SIZE]; // need to be continuous
uint8_t rec_buffer[REC_DESCRIPTOR_QUEUE_SIZE * REC_BUF_SIZE]; // need to be continuous


struct e1000_tail* tdt;
struct e1000_tail* rdt;


// adapted from https://github.com/gatsbyd/mit_6.828_jos_2018/blob/9ab80fb35863c30c2ec1bcd4bfaa9ea2bd9ee4bc/kern/e1000.c#L84
uint32_t E1000_MAC[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

static void
get_ra_address(uint32_t mac[], uint32_t *ral, uint32_t *rah)
{
       uint32_t low = 0, high = 0;
       int i;

       for (i = 0; i < 4; i++) {
               low |= mac[i] << (8 * i);
       }

       for (i = 4; i < 6; i++) {
               high |= mac[i] << (8 * i);
       }

       *ral = low;
       *rah = high | E1000_RAH_AV;
}

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


    // receive initialization
    for(int i = 0; i < REC_DESCRIPTOR_QUEUE_SIZE; i++) {
        rec_decs_queue[i].addr_low = PADDR(rec_buffer + i * REC_BUF_SIZE);
        rec_decs_queue[i].addr_high = 0;
        rec_decs_queue[i].length = REC_BUF_SIZE;
        rec_decs_queue[i].dd = 0; // mark as receive
    }

    // int i = 0;
    // uint32_t *begin = (uint32_t*)(rec_decs_queue + i);
    // // if(rec_decs_queue[i].dd == 1) {
    //     // cprintf("index %d --- currnet %d :: ", i, next);
    // for(int j = 0; j < 4; j++) {
    //     cprintf("%08x ", *(begin + j));
    // }
    // panic("break");

    assert(sizeof(rec_decs_queue[0]) == 16);

    // setup rar for mac address, why this not work?
    // struct e1000_rar* rar = (struct e1000_rar *)(e1000c + E1000_RA);
    // rar->l = 0x12005452; // little endian
    // rar->h = 0x5634;
    // rar->av = 1;
    // assert(sizeof(*rar) == 8);

    uint32_t * ra = (uint32_t *)(e1000c + E1000_RA);
    uint32_t ral, rah;
    get_ra_address(E1000_MAC, &ral, &rah);
    ra[0] = ral;
    ra[1] = rah;

    struct e1000_rdba * rdba = (struct e1000_rdba *)(e1000c + E1000_RDBAL);
    rdba->l = PADDR(rec_decs_queue);
    rdba->h = 0;


    *(uint32_t *)(e1000c + E1000_RDLEN) = REC_DESCRIPTOR_QUEUE_SIZE * 16; // 16 descritor size
    
    
    rdt = (struct e1000_tail*)(e1000c + E1000_RDT);
    struct e1000_tail* rdh = (struct e1000_tail*)(e1000c + E1000_RDH); 
    rdh->p = 0;
    rdt->p = REC_DESCRIPTOR_QUEUE_SIZE - 1;

    // struct e1000_rctl* rctl = (struct e1000_rctl* )(e1000c + E1000_RCTL);
    // rctl->en = 1;
    // rctl->bsize = 0; // 00b for 2048 byte buffer
    // rctl->bam = 1;
    // rctl->secrc = 1; // strip crc
    // rctl->lpe = 0; // disable long packet

    uint32_t *rctl = (uint32_t *)(e1000c + E1000_RCTL);
    *rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC;
    cprintf("size of rctl %d \n", sizeof(*rctl));
    assert(sizeof(*rctl) == 4);
    
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

// receive packet
int e1000_receive_one_packet(void* packet, uint32_t size) {
    uint32_t next = (rdt->p + 1) % REC_DESCRIPTOR_QUEUE_SIZE;
    if(rec_decs_queue[next].dd == 0) { // empty 
        for(int i = 0; i < REC_DESCRIPTOR_QUEUE_SIZE; i ++) {
            uint32_t *begin = (uint32_t*)(rec_decs_queue + i);
            if(rec_decs_queue[i].dd == 1) {
                cprintf("index %d --- currnet %d :: ", i, next);
                for(int j = 0; j < 4; j++) {
                    cprintf("%08x ", *(begin + j));
                }
                cprintf("\n");
            }
            // cprintf("%x, %d, %d \n", rec_decs_queue[i].addr_low, rec_decs_queue[i].length, rec_decs_queue[i].dd);
        }
        // panic("empty queue %d", next);
        return -1;
    }
    if( rec_decs_queue[next].error != 0) { // error occur
        return -2;
    }
    // panic("receive packet");
    // 用户进程保证有足够的空间
    uint32_t packet_len = rec_decs_queue[next].length;
    assert(packet_len <= size);
    memmove(packet, (void *)KADDR(rec_decs_queue[next].addr_low), packet_len);
    rec_decs_queue[next].dd = 0;
    rdt->p = next;
    return packet_len;
}