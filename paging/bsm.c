/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>
#include <mem.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    STATWORD ps;
    disable(ps);

    int i;
    for(i=0; i<NBS; i++) {
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_pid = 0;
        bsm_tab[i].bs_vpno = 4096;
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_sem = 0;
        bsm_tab[i].has_vheap = 0;
    }
    restore(ps);
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    STATWORD ps;
    disable(ps);

    int i;
    for(i=0; i<NBS; i++) {
        if(bsm_tab[i].bs_status == BSM_UNMAPPED) {
            *avail = i;
            restore(ps);
            return OK;
        }
    }
    restore(ps);
    return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    STATWORD ps;
    disable(ps);

    if(i > -1 && i < NBS)
    {
        bsm_tab[i].bs_status = BSM_UNMAPPED;
        bsm_tab[i].bs_pid = 0;
        bsm_tab[i].bs_vpno = 4096;
        bsm_tab[i].bs_npages = 0;
        bsm_tab[i].bs_sem = 0;
        bsm_tab[i].has_vheap = 0;

        restore(ps);
        return OK;
    }

    restore(ps);
    return SYSERR;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    STATWORD 	ps;
	disable(ps);
	
	int i = 0;
	for (i = 0; i < NBS; i++)
	{
		if (bsm_tab[i].bs_status == BSM_MAPPED && bsm_tab[i].bs_pid == pid)
		{
			*store = i;
            *pageth = (vaddr/NBPG) - bsm_tab[i].bs_vpno;
			restore(ps);
			return OK;
		}				
	}
	restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    STATWORD 	ps;
	disable(ps);

    if(bsm_tab[source].has_vheap == 0 || (bsm_tab[source].has_vheap == 1 && bsm_tab[source].bs_pid == pid))
    {
        if(npages>256 || npages<0) {
            restore(ps);
            return SYSERR;
        }

        bsm_tab[source].bs_pid = pid;
        bsm_tab[source].bs_vpno = vpno;
        bsm_tab[source].bs_npages = npages;
        bsm_tab[source].bs_status = BSM_MAPPED;

        proctab[pid].store = source;
        proctab[pid].vhpno = 4096;
        proctab[pid].vhpnpages = npages;
    }
    restore(ps);
    return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    STATWORD ps;
    disable(ps);

    int store, pageth;
    bsm_lookup(pid, vpno, &store, &pageth);

    bsm_tab[store].bs_status = BSM_UNMAPPED;
	bsm_tab[store].bs_pid = 0;
	bsm_tab[store].bs_vpno = 4096;
	bsm_tab[store].bs_npages = 0;
    bsm_tab[store].has_vheap = 0;

    restore(ps);
    return OK;
}


