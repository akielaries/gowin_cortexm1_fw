.syntax unified
.thumb
.global PendSV_Handler
.type PendSV_Handler, %function

.extern current_thread
.extern scheduler_next


PendSV_Handler:
    cpsid   i
    push    {r4, lr}            // save lr AND pad to keep 8-byte alignment

    ldr     r2, =current_thread
    ldr     r3, [r2]
    cmp     r3, #0
    beq     skip_save

    mrs     r0, psp
    subs    r0, r0, #32

    movs    r1, r0
    adds    r1, #16
    stm     r1!, {r4-r7}        // save real r4-r7 at +16

    mov     r4, r8
    mov     r5, r9
    mov     r6, r10
    mov     r7, r11
    stm     r0!, {r4-r7}        // save r8-r11 at +0

    subs    r0, r0, #16         // r0 back to frame base
    str     r0, [r3]            // t->sp = r0

skip_save:
wait_for_ready:
    cpsie   i
    wfi
    cpsid   i
    bl      scheduler_next
    cmp     r0, #0
    beq     wait_for_ready

    ldr     r2, =current_thread
    str     r0, [r2]            // current_thread = next
    ldr     r0, [r0]            // r0 = next->sp

    ldm     r0!, {r4-r7}        // restore r8-r11 (stored at low addr)
    mov     r8, r4
    mov     r9, r5
    mov     r10, r6
    mov     r11, r7

    ldm     r0!, {r4-r7}        // restore r4-r7

    msr     psp, r0
    cpsie   i
    pop     {r4, pc}            // restore saved r4, and EXC_RETURN -> pc
