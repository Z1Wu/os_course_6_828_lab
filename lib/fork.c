// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

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
    void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	if ( !((err & FEC_WR) && (uvpd[PDX(addr)] & PTE_P) && // present in page directory and is a write operation
			(uvpt[PGNUM(addr)] & PTE_P) // present corespponded page table
			&& (uvpt[PGNUM(addr)] & PTE_COW)) // fault on a copy on write page
	) {
		panic("not copy-on-write");	
	}

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
    // int r;
    // uintptr_t addr = pn * PGSIZE;
    // thisenv->env_pgdir
    // envid_t cur_eid = sys_getenvid();
    // if( uvpt[pn] & PTE_W || uvpt[pn] & PTE_COW ) { // if that page is copy-on-write or write
    //     // 让子进程的内存空间映射到父进程对应的页中
    //     r = sys_page_map(cur_eid, (void*)addr, envid, (void*)addr, PTE_COW | PTE_U | PTE_P);
    //     if( r < 0) panic("error c2d");
    //     // 修改父进程的内存空间
    //     r = sys_page_map(cur_eid, (void*)addr, cur_eid, (void*)addr, PTE_COW | PTE_U | PTE_P);
    //     if( r < 0) panic("error d2d");
    // } else { // 只读的页
    //     r = sys_page_map(cur_eid, (void*)addr, envid, (void*)addr, PTE_U | PTE_P);
    //     if( r < 0) panic("error d2d_r");
    // }

	// LAB 4: Your code here.
	void *addr = (void*) (pn*PGSIZE);
    // cprintf("")
	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
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
envid_t
fork(void)
{
	set_pgfault_handler(pgfault);

	envid_t envid;
	uint32_t addr;
	// Allocate a new child environment.
	// The kernel will initialize it with a copy of our register state,
	// so that the child will appear to have called sys_exofork() too -
	// except that in the child, this "fake" call to sys_exofork()
	// will return 0 instead of the envid of the child.
	envid = sys_exofork();
	if (envid == 0) {
		// this will never be called? 
		thisenv = &envs[ENVX(sys_getenvid())];
        // set_pgfault_handler(pgfault);
		return 0;
	}
	if (envid < 0)
		panic("sys_exofork: %e", envid);

	for (addr = 0; addr < USTACKTOP; addr += PGSIZE)
		if (
			(uvpd[PDX(addr)] & PTE_P) && 
			(uvpt[PGNUM(addr)] & PTE_P) && 
			(uvpt[PGNUM(addr)] & PTE_U)
		) {
			// 需要执行子进程到父进程的内存映射的页
			duppage(envid, PGNUM(addr));
		}
    // 子进程的 excepetion stack 不和父进程的共同使用
	if (sys_page_alloc(envid, (void *)(UXSTACKTOP-PGSIZE), PTE_U|PTE_W|PTE_P) < 0) 
		panic("1");

	extern void _pgfault_upcall(); // 这个函数定义在汇编文件 pfentry.S 中
	if(sys_env_set_pgfault_upcall(envid, _pgfault_upcall) < 0) { 
		// 这个子进程的 pgfault 回调的注册*必须*在由父进程帮忙注册 （子进程第一个 page fault发生的时机)
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
