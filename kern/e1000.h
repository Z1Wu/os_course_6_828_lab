#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include "pci.h"
#include "e1000_hw.h"
#include "inc/string.h"

#define TX_DESCRIPTOR_QUEUE_SIZE 16
#define TX_BUF_SIZE 1518

int e1000_attachfn (struct pci_func *pcif);
int send_one_packet(void* packet, uint32_t size);

// struct tx_desc
// {
// 	// uint64_t addr;
//     struct addr {
//         uint32_t low;
//         uint32_t high;
//     } addr;
// 	uint32_t length: 16;
// 	uint32_t cso:8;
	
//     struct cmd {
//         uint32_t eop : 1;
//         uint32_t ifcs : 1;
//         uint32_t ic : 1;
//         uint32_t rs : 1;
//         uint32_t other : 4;
//     } cmd;
//     struct status {
//         uint32_t dd : 1;
//         uint32_t ec : 1;
//         uint32_t lc : 1;
//         uint32_t rsv : 1;
//     } status;
// 	uint32_t css : 8;
// 	uint32_t special:16;
// };

struct tx_desc
{
	// uint64_t addr;
    uint32_t addr_low : 32;
    uint32_t addr_high : 32;
	uint32_t length: 16;
	uint32_t cso:8;
    // CMD
    uint32_t eop : 1;
    uint32_t ifcs : 1;
    uint32_t ic : 1;
    uint32_t rs : 1;
    uint32_t other : 4;
    // STA
    uint32_t dd : 1;
    uint32_t ec : 1;
    uint32_t lc : 1;
    uint32_t rsv : 1;
    // RSV
    uint32_t rsv1 : 4;
    // CSS
	uint32_t css : 8;
    // SPECIAL
	uint32_t special:16;
};

struct e1000_tdba
{
    uint32_t l;
    uint32_t h; // order
};

struct e1000_tdlen
{
    uint32_t len; // order
};

struct e1000_head
{
    uint16_t p;
};

struct e1000_tail
{
    uint16_t p;
};

// TODO: struct of e1000_tctl
// 31: 26 - 25: 22 - 21: 12 - 11 :4 - 3:0
// Reser  - CNTL   - COLD   - CT    - CNTL
struct e1000_tctl
{
    // ctl bits
    uint32_t rsv1 : 1;
    uint32_t en : 1;
    uint32_t rsv2 : 1;
    uint32_t psp : 1;
    uint32_t ct: 8;
    // cold
    uint32_t cold: 10;
    uint32_t other: 10;
};

struct e1000_tipg
{
    uint16_t ipgt: 10;
    uint16_t ipgr1: 10;
    uint16_t ipgr2: 10;
    uint8_t rsv: 2;
};

#endif  // SOL >= 6