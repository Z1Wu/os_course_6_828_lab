#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include "pci.h"
#include "e1000_hw.h"
#include "inc/string.h"

#define TX_DESCRIPTOR_QUEUE_SIZE 16
#define REC_DESCRIPTOR_QUEUE_SIZE 128
#define TX_BUF_SIZE 1518
#define REC_BUF_SIZE 2048

#define E1000_RX_EMPTY -10


int e1000_attachfn (struct pci_func *pcif);
int send_one_packet(void* packet, uint32_t size);
int e1000_receive_one_packet(void* packet, uint32_t size);

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


struct rec_desc
{
	// uint64_t addr;
    uint32_t addr_low : 32;
    uint32_t addr_high : 32;
	uint32_t length: 16;
	uint32_t check_sum:16;
    
    // status
	uint32_t dd:1;
    uint32_t eop:1;
	uint32_t status_other:6;
    
    // error
	uint32_t error:8;

    // SPECIAL
	uint32_t special:16;
};

struct e1000_rdba
{
    uint32_t l;
    uint32_t h; // order
};

struct e1000_rar // mac address
{
    uint32_t l: 32;
    uint32_t h: 16;
    uint32_t as: 2;
    uint32_t rsv: 13;
    uint32_t av: 1;
};

struct e1000_rctl
{
    // ctl bits
    // first 16 bits
    uint32_t rsv1 : 1;
    uint32_t en : 1;
    uint32_t sbp : 1;
    uint32_t upe : 1; // unicast enable
    uint32_t mpe: 1; // multi-cast enable
    uint32_t lpe: 1; // long-packet enable
    uint32_t lbm: 2; // loopback enable
    uint32_t rdmts: 2; // long-packet enable
    uint32_t rsv2: 2; // long-packet enable
    uint32_t mo: 2; // mutli-cast offset -> 00b
    uint32_t rsv3: 1;
    uint32_t bam: 1;
    // last 16
    uint32_t bsize: 2; // 00 for 2048 bytes
    uint32_t others: 7;
    uint32_t bsex: 1; // 0  default
    uint32_t secrc: 1; // strip crc
    uint32_t rsv4: 5; // 
};

#endif  // SOL >= 6