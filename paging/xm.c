/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);

  if(bsm_tab[(int)source].has_vheap == 1  && bsm_tab[(int)source].bs_pid != currpid)
  {
    restore(ps);
    kill(currpid);
    return SYSERR;
  }

  if(((int)source > -1 && (int)source < 8 && npages >= 1 && npages <= 256 ))
  {
    if (bsm_map(currpid,virtpage,source,npages) != SYSERR)
		{
      restore(ps);
      return OK;
		}
  }

  kprintf("Some failure in xmmap!");
  restore(ps);
  return SYSERR;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  STATWORD ps;
  disable(ps);

  bsm_unmap(currpid, virtpage, 0);

  restore(ps);
  return OK;
}
