// User-level IPC library routines

#include <inc/lib.h>

// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'from_env_store' is nonnull, then store the IPC sender's envid in
//	*from_env_store.
// If 'perm_store' is nonnull, then store the IPC sender's page permission
//	in *perm_store (this is nonzero iff a page was successfully
//	transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
// Otherwise, return the value sent by the sender
//
// Hint:
//   Use 'thisenv' to discover the value and who sent it.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value, since that's
//   a perfectly valid place to map a page.)
int32_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	// LAB 4: Your code here.
    int r;
    if(pg == NULL) {
        // 传输一个 非法 的地址代表不接受 mapping page 
        r = sys_ipc_recv((void *)(UTOP + 1));
    } else {
        r = sys_ipc_recv(pg);
    }
    if(r != 0) { // error occur
        if (from_env_store) *from_env_store = 0;
        if (perm_store) *perm_store = 0;
        return r;
    }
    if(pg && perm_store) {
        *perm_store = thisenv->env_ipc_perm;
    }
    if(from_env_store) {
        *from_env_store = thisenv->env_ipc_from;
    }
    return  thisenv->env_ipc_value;
}

// Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
//
// Hint:
//   Use sys_yield() to be CPU-friendly.
//   If 'pg' is null, pass sys_ipc_try_send a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
void
ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
{
	// LAB 4: Your code here.
    int ret;
    if( pg == NULL) {
        pg = (void*) UTOP + 1;
    }
    while(1) {
        ret = sys_ipc_try_send(to_env, val, pg, perm);
        if (ret == 0) {
            return;
        } else if ( ret != -E_IPC_NOT_RECV ) {
            panic("error occur %d", ret);
        }
        // 到这里说明目前对应的进程没有期望接收信息
        cprintf("target %d don't expect receive massage give up cpu\n", to_env);
        sys_yield(); // TODO: 为什么需要放弃 CPU，同时这个 send 为什么要放在一个 loop 里面?
    }
	panic("ipc_send not implemented");
}

// Find the first environment of the given type.  We'll use this to
// find special environments.
// Returns 0 if no such environment exists.
envid_t
ipc_find_env(enum EnvType type)
{
	int i;
	for (i = 0; i < NENV; i++)
		if (envs[i].env_type == type)
			return envs[i].env_id;
	return 0;
}
