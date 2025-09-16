#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>

int get_bs(bsd_t bs_id, unsigned int npages) {
  /* requests a new mapping of npages with ID map_id */
  STATWORD ps;
  disable(ps);

  if ((int)bs_id > -1 && (int)bs_id < NBS && npages > 0 && npages <= 256 && bsm_tab[(int)bs_id].has_vheap == 0)
  {
    if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED)
	  {
		  bsm_tab[bs_id].bs_status = BSM_MAPPED;
		  bsm_tab[bs_id].bs_pid = currpid;
		
		  restore(ps);
		  return npages;		
	  }
	  else
	  {
		  restore(ps);
		  return bsm_tab[bs_id].bs_npages;
	  }
  }
  
  kprintf("Backing store cannot be given as it is assigned to vcreate process!\n");
  kill(currpid);
  restore(ps);
  return SYSERR;
}


