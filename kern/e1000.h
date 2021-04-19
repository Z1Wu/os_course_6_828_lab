#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include "pci.h"
#include "e1000_hw.h"

#define TX_DESCRIPTOR_QUEUE_SIZE 48
#define TX_BUF_SIZE 1518

int e1000_attachfn (struct pci_func *pcif);


struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	
    uint8_t cmd;
    struct status {
        uint8_t dd : 1;
        uint8_t ec : 1;
        uint8_t lc : 1;
        uint8_t rsv : 1;
    } status;
	uint8_t css;
	uint16_t special;
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