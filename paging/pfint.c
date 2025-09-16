/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  STATWORD ps;
  disable(ps);

  // kprintf("Page Fault Handler called!\n");
  unsigned long vpage;
  vpage = read_cr2();

  virt_addr_t *vaddr;
  vaddr = (virt_addr_t*)&vpage; //bit field map

  unsigned long pdbr;
  pt_t *pte;
	pd_t *pde;
  pdbr = proctab[currpid].pdbr;

  pde = pdbr + (vaddr->pd_offset)*sizeof(pd_t);

  int new_frame;
  //if no directory
  if(!pde->pd_pres)
  {
    get_frm(&new_frame);
    pte = (pt_t*)((FRAME0 + new_frame) * NBPG);
    int i;
    //page table init
    for (i = 0; i < 1024; i++)
		{
			(pte + i)->pt_pres = 0;
			(pte + i)->pt_write = 0;
			(pte + i)->pt_user = 0;
			(pte + i)->pt_pwt = 0;
			(pte + i)->pt_pcd = 0;
			(pte + i)->pt_acc = 0;
			(pte + i)->pt_dirty = 0;
			(pte + i)->pt_mbz = 0;
			(pte + i)->pt_global = 0;
			(pte + i)->pt_avail = 0;
			(pte + i)->pt_base = 0;	
		}

    /* Edit frame tab values */
    frm_tab[new_frame].fr_status = FRM_MAPPED;
		frm_tab[new_frame].fr_type = FR_TBL;
		frm_tab[new_frame].fr_pid = currpid;

    pde->pd_pres = 1;
		pde->pd_write = 1;
		pde->pd_user = 1;
		pde->pd_pwt = 0;
		pde->pd_pcd = 0;
		pde->pd_acc = 1;
		pde->pd_mbz = 0;
		pde->pd_fmb = 0;
		pde->pd_global = 0;
		pde->pd_avail = 0;
		pde->pd_base = FRAME0 + new_frame;
  }

  //directory now exists
  pte = (pt_t*) (pde->pd_base * NBPG + vaddr->pt_offset * sizeof(pt_t));
  if(!pte->pt_pres)
  {
    int new_pte;
    get_frm(&new_pte);

    pte->pt_pres = 1;
		pte->pt_write = 1;
		pte->pt_base = FRAME0 + new_pte;

    /* Edit frame tab values */
    frm_tab[new_pte].fr_status = FRM_MAPPED;
		frm_tab[new_pte].fr_type = FR_PAGE;
		frm_tab[new_pte].fr_pid = currpid;
		frm_tab[new_pte].fr_vpno = vpage/NBPG;
    frm_tab[pde->pd_base - FRAME0].fr_refcnt++;

    int store, pageth;

    bsm_lookup(currpid,vpage,&store,&pageth);
		read_bs((char*)((FRAME0 + new_pte) * NBPG),store,pageth);
  }

  write_cr3(pdbr);	
	restore(ps);
	return OK;
}


