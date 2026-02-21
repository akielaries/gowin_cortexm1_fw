.syntax unified
.thumb
.global PendSV_Handler
.type PendSV_Handler, %function

.extern current_thread
.extern scheduler_next

PendSV_Handler:
    cpsid   i
    push    {lr}

    /* Check if current_thread is NULL — skip save if so */
    ldr     r2, =current_thread
    ldr     r3, [r2]
    cmp     r3, #0
    beq     skip_save

    /* Save current thread */
    mrs     r0, psp
    movs    r1, #32
    subs    r0, r0, r1

    movs    r1, #16
    adds    r1, r0, r1
    stm     r1!, {r4-r7}

    mov     r4, r8
    mov     r5, r9
    mov     r6, r10
    mov     r7, r11
    stm     r0!, {r4-r7}

    movs    r1, #16
    subs    r0, r0, r1
    str     r0, [r3]

skip_save:
    bl      scheduler_next
    str     r0, [r2]
    ldr     r0, [r0]

    ldm     r0!, {r4-r7}
    mov     r8, r4
    mov     r9, r5
    mov     r10, r6
    mov     r11, r7

    ldm     r0!, {r4-r7}

    msr     psp, r0
    cpsie   i
    pop     {pc}
