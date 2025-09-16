/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int sc_head = -1;
extern int page_replace_policy;
extern int debug_on;

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  STATWORD 	ps;
	disable(ps);
	
	sc_head = -1;

	int i = 0;
	for (i = 0; i < NFRAMES; i++)
	{
		frm_tab[i].fr_status = FRM_UNMAPPED;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_vpno = 0;
		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_type = FR_PAGE;
		frm_tab[i].fr_dirty = 0;
		frm_tab[i].next_fr = -1;		
		frm_tab[i].prev_fr = -1;
	}

	restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  STATWORD 	ps;
	disable(ps);

	int i = 0;
	for (i = 0; i < NFRAMES; i++)
	{
		if (frm_tab[i].fr_status == FRM_UNMAPPED) /* 1st unmapped frame  */
		{
			*avail = i;
			insert_frame(i);
			restore(ps);
			return OK;
		}				
	}

	int frame_id;
	frame_id = sc_policy();

	if (frame_id > -1 && frame_id < NFRAMES)
	{
		free_frm(frame_id);
		*avail = frame_id;
		insert_frame(frame_id);
		restore(ps);
		return(OK);
	}	
		
  restore(ps);
  return SYSERR;
}

void insert_frame(int i) {
	if(sc_head == -1) {
		frm_tab[i].next_fr = i;
		frm_tab[i].prev_fr = i;
		sc_head = i;
	}
	else {
		int tail = frm_tab[sc_head].prev_fr;

		frm_tab[i].next_fr = sc_head;
		frm_tab[i].prev_fr = tail;
		frm_tab[sc_head].prev_fr = i;
		frm_tab[tail].next_fr = i;
	}
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
	unsigned int a = frm_tab[i].fr_vpno*NBPG;
	int store, pageth;
	if(bsm_lookup(frm_tab[i].fr_pid, a, &store, &pageth) == SYSERR) {
		kill(frm_tab[i].fr_pid);
		return SYSERR;
	}

	unsigned long pdbr = proctab[currpid].pdbr;
	int ptno = frm_tab[i].fr_vpno/1024;
	int pgno = frm_tab[i].fr_vpno%1024;
	pd_t* pde = (pd_t*) (pdbr + ptno*sizeof(pd_t));
	pt_t* pte = (pt_t*) (pde->pd_base*4096 + pgno*sizeof(pt_t));

	write_bs((char *)((FRAME0 + i) * NBPG), store, pageth);

	if(frm_tab[i].fr_pid = currpid)
	{
		__asm__("invlpg (%0)" : : "r" (a) : "memory");
	}

	pte->pt_pres = 0;
	frm_tab[pde->pd_base - FRAME0].fr_refcnt--;

	if (frm_tab[pde->pd_base - FRAME0].fr_refcnt == 0)
	{
		pde->pd_pres = 0;
		frm_tab[pde->pd_base - FRAME0].fr_status = FRM_UNMAPPED;
		frm_tab[pde->pd_base - FRAME0].fr_type = FR_PAGE;
		frm_tab[pde->pd_base - FRAME0].fr_pid = -1;
		frm_tab[pde->pd_base - FRAME0].fr_vpno = 4096;	
	} 

	return OK;
}

int sc_policy() {
	STATWORD ps;
	disable(ps);

	int current_frame = sc_head;

	while(current_frame != -1) {

		fr_map_t* frame = &frm_tab[current_frame];
		
		int vpno = frame->fr_vpno;
		int ptno = vpno/1024;
		int pgno = vpno%1024;
		unsigned long pdbr = proctab[frm_tab[current_frame].fr_pid].pdbr;
		pd_t* pde = (pd_t*) (pdbr + ptno*sizeof(pd_t));
		pt_t* pte = (pt_t*) (pde->pd_base*4096 + pgno*sizeof(pt_t));
		
		if(frm_tab[current_frame].fr_type == FR_PAGE && pte->pt_acc == 0) {
			
			sc_head = frm_tab[current_frame].next_fr;
			int prev = frm_tab[current_frame].prev_fr;
			int next = frm_tab[current_frame].next_fr;
			frm_tab[prev].next_fr = next;
			frm_tab[next].prev_fr = prev;

			frm_tab[current_frame].next_fr = -1;
            frm_tab[current_frame].prev_fr = -1;

			if(debug_on)
			{
				kprintf("\nReplacing frame: %d\n",current_frame);
			}

			restore(ps);
            return current_frame;
		}
		else {
			pte->pt_acc = 0;
			current_frame = frm_tab[current_frame].next_fr;
			
		}
	}

	frm_tab[current_frame].next_fr = -1;
	restore(ps);
	return -1;
}
