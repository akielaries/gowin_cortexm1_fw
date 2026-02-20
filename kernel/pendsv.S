.syntax unified
.thumb
.global PendSV_Handler
.type PendSV_Handler, %function

.extern current_thread
.extern scheduler_next

PendSV_Handler:
    cpsid   i                  /* Disable interrupts */

    /* --- Save current thread --- */
    mrs     r0, psp            /* r0 = current PSP */
    isb
    mov     sp, r0

    /* Save r4-r7 */
    push    {r4, r5, r6, r7}

    /* Save r8-r11 via r4-r7 temporaries */
    mov     r4, r8
    mov     r5, r9
    mov     r6, r10
    mov     r7, r11
    push    {r4, r5, r6, r7}

    /* Store updated SP to current_thread->sp */
    ldr     r2, =current_thread
    ldr     r3, [r2]           /* r3 = current_thread */
    mov     r0, sp
    str     r0, [r3]

    /* --- Pick next thread --- */
    bl      scheduler_next
    str     r0, [r2]           /* current_thread = next */

    /* Load next thread SP into SP */
    ldr     r0, [r0]           /* r0 = next->sp */
    mov     sp, r0

    /* Restore r8-r11 via r4-r7 */
    pop     {r4, r5, r6, r7}
    mov     r8, r4
    mov     r9, r5
    mov     r10, r6
    mov     r11, r7

    /* Restore r4-r7 */
    pop     {r4, r5, r6, r7}

    /* Update PSP with new thread SP */
    mov     r0, sp
    msr     psp, r0

    cpsie   i                  /* Re-enable interrupts */
    bx      lr                 /* Return to thread mode */

