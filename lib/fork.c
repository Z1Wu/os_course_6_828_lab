// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

// #define MAXFD		32
// // Bottom of file descriptor area
// #define FDTABLE		0xD0000000
// // Bottom of file data area.  We reserve one data page for each FD,
// // which devices can use if they choose.
// #define FILEDATA	(FDTABLE + MAXFD*PGSIZE)

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//

// extern pte_t* uvpd, *uvpt;

static void
pgfault(struct UTrapframe *utf)
{
	// void *addr = (void *) utf->utf_fault_va;
	// uint32_t err = utf->utf_err;
	// int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
    // if( !(err & FEC_WR) || !(uvpt[PGNUM(addr)] & PTE_COW)) {
    //     panic("error when hanling pgfault %x", err);
    // }

	// // LAB 4: Your code here.

	// // Allocate a new page, map it at a temporary location (PFTEMP),
	// // copy the data from the old page to the new page, then move the new
	// // page to the old page's address.
	// // Hint:
	// //   You should make three system calls.
    // envid_t eid = thisenv->env_id;
    // r = sys_page_alloc(eid, PFTEMP, PTE_W | PTE_U | PTE_P);
    // if (r < 0) panic("sys_page_alloc");
    // memmove((void *)PFTEMP, addr, PGSIZE);
    // r = sys_page_map(eid, PFTEMP, eid, addr, PTE_W | PTE_U | PTE_P);
    // if (r < 0) panic("sys_page_map");
    // r = sys_page_unmap(eid, PFTEMP);
    // if (r < 0) panic("sys_page_unmap");

    void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	if (!((err & FEC_WR) && (uvpd[PDX(addr)] & PTE_P) && 
			(uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_COW)))
		panic("not copy-on-write");

	addr = ROUNDDOWN(addr, PGSIZE);
	if (sys_page_alloc(0, PFTEMP, PTE_W|PTE_U|PTE_P) < 0)
		panic("sys_page_alloc");
	memcpy(PFTEMP, addr, PGSIZE);
	if (sys_page_map(0, PFTEMP, 0, addr, PTE_W|PTE_U|PTE_P) < 0)
		panic("sys_page_map");
	if (sys_page_unmap(0, PFTEMP) < 0)
		panic("sys_page_unmap");
	return;
}


//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (TODO:Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?) 
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// LAB 4: Your code here.
	void *addr = (void*) (pn*PGSIZE);
	if ((uvpt[pn] & PTE_SHARE)) { // 在 shared 区域
		cprintf("fork share addr %8x \n", addr);
		if (sys_page_map(0, addr, 0, addr, (uvpt[pn] & PTE_SYSCALL) | PTE_SHARE) < 0) {
			panic("error occur when set current shared pte");
		}
		if (sys_page_map(0, addr, envid, addr, (uvpt[pn] & PTE_SYSCALL) | PTE_SHARE) < 0) {
			panic("error occur when set target shared pte");
		}
	} else if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) { 
		// 如果当前的 env addr 对应的 pte 是可写/ cow 的，需要修改 map 。
		if (sys_page_map(0, addr, envid, addr, PTE_COW|PTE_U|PTE_P) < 0)
			panic("2");
		if (sys_page_map(0, addr, 0, addr, PTE_COW|PTE_U|PTE_P) < 0)
			panic("3");
	} else {
        sys_page_map(0, addr, envid, addr, PTE_U|PTE_P);
    }
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
// envid_t
// fork(void)
// {
// 	// LAB 4: Your code here.
//     envid_t envid;
// 	uintptr_t addr;
// 	int r;
//     extern unsigned char end[];

// 	// Allocate a new child environment.
// 	// The kernel will initialize it with a copy of our register state,
// 	// so that the child will appear to have called sys_exofork() too -
// 	// except that in the child, this "fake" call to sys_exofork()
// 	// will return 0 instead of the envid of the child.
// 	envid = sys_exofork();
// 	if (envid < 0)
// 		panic("sys_exofork: %e", envid);
// 	if (envid == 0) {
//         envid_t eid = sys_getenvid();
// 		thisenv = &envs[ENVX(eid)];
//         // 为子进程注册 page fault handler
//         cprintf("new child env [%08x], parent env[%08x]", eid, envid);
//         set_pgfault_handler(pgfault);
// 		return 0;
// 	}

// 	// for (addr = (uint8_t*) UTEXT; addr < end; addr += PGSIZE) {
// 	// 	duppage(envid, addr);
//     // }
    
//     cprintf("copying pgdir \n");
//     // cprintf("%x\n", USTACKTOP);
//     for (addr = 0; addr < USTACKTOP; addr += PGSIZE) { // 在 env_alloc 的时候已经分配好了 UTOP 之上的 mapping
//         // 检查这一页在父进程中是否已经被分配,如果是分配好的，则让子进程也映射到该页
//         // 需要注意这里需要先检查 uvpd 中的 pde 是否存在之后在去检验对应的 pte
//         if ((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_U)) {
//             cprintf("handle page: %d \n", PGNUM(addr));
// 			duppage(envid, PGNUM(addr));
// 		}
//     }
//     // for (addr = 0; addr < USTACKTOP; addr += PGSIZE)
// 	// 	if ((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P)
// 	// 		&& (uvpt[PGNUM(addr)] & PTE_U)) {
// 	// 		duppage(envid, PGNUM(addr));
// 	// 	}

//     cprintf("allocate expcetion stack");
//     // 分配一个页给 user exception stack
//     r = sys_page_alloc(envid, (void*)(UXSTACKTOP - PGSIZE), PTE_W | PTE_P | PTE_U);
//     if (r < 0) panic("error when allocate a user exception stack");

//     // todo： 为什么这里是
//     // extern void _pgfault_upcall();
// 	// sys_env_set_pgfault_upcall(envid, _pgfault_upcall);

// 	// Start the child environment running
//     cprintf("set chiild runnable");
// 	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
// 		panic("sys_env_set_status: %e", r);

// 	return envid;
// }

envid_t
fork(void)
{
	set_pgfault_handler(pgfault);

	envid_t envid;
	uint32_t addr;
	envid = sys_exofork();
	if (envid == 0) {
		// panic("child");
		thisenv = &envs[ENVX(sys_getenvid())];
        // set_pgfault_handler(pgfault);
		return 0;
	}
	// cprintf("sys_exofork: %x\n", envid);
	if (envid < 0)
		panic("sys_exofork: %e", envid);

	for (addr = 0; addr < USTACKTOP; addr += PGSIZE)
		if ((uvpd[PDX(addr)] & PTE_P) && (uvpt[PGNUM(addr)] & PTE_P)
			&& (uvpt[PGNUM(addr)] & PTE_U)) { // 需要执行子进程到父进程的内存映射的页
			duppage(envid, PGNUM(addr));
		}
    // 父进程为子进程分配一个新的页用于 exception stack
	if (sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_U|PTE_W|PTE_P) < 0) 
		panic("1");

	extern void _pgfault_upcall(); // 这个函数定义在汇编文件 pfentry.S 中
	if(sys_env_set_pgfault_upcall(envid, _pgfault_upcall) < 0) { 
        // 关于子进程在何处注册对应的 pgfault 回调
        panic("setup pgfault upcall error");
    }

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0)
		panic("sys_env_set_status");

	return envid;
}


// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
