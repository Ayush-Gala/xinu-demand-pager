/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
    popl pferrcode
    cli
    pushfl
    pushal
    call pfint
    popal
    popfl
    sti
    iret


