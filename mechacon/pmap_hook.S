.syntax unified
.cpu arm7tdmi
.thumb
.section .text

.include "prof.inc"
.equ ARMED, 0x31344151
.global pmap_entry
.global ncmd_entry
.thumb_func
pmap_entry:
    ldr r0, =SCMD_REQ
    ldr r1, [r0]
    ldr r2, =0x423256a4
    cmp r1, r2
    bne original
    ldr r2, [r0, #4]
    ldrb r1, [r0, #8]
    cmp r1, #0
    beq ram_word
    cmp r1, #1
    beq half_write
    cmp r1, #2
    bne reject
    ldr r0, =WINDOW
    ldr r1, [r0]
    ldr r3, =ARMED
    cmp r1, r3
    bne reject
    push {lr}
    adds r0, #9
    adds r3, r0, #0
    adds r0, r2, #0
    bl call_r3
    adds r2, r0, #0
    pop {r3}
    mov lr, r3
    b respond_48
half_write:
    lsrs r1, r2, #16
    lsls r3, r1, #31
    bne reject
    ldr r3, =WINDOW_SIZE
    cmp r1, r3
    bhs reject
    ldr r0, =WINDOW
    adds r0, r0, r1
    strh r2, [r0]
    ldrh r2, [r0]
    lsls r1, r1, #16
    orrs r2, r1
    b respond_48
ram_word:
    lsls r1, r2, #30
    bne reject
    movs r1, #1
    lsls r1, r1, #25
    subs r1, r2, r1
    lsrs r1, r1, #14
    bne reject
    ldr r2, [r2]
respond_48:
    movs r0, #0x48
    b respond
reject:
    movs r2, #0
    movs r0, #0xe1
respond:
    ldr r1, =HOST_FIFO
    strb r0, [r1]
    movs r0, #8
value_loop:
    strb r2, [r1]
    lsrs r2, r2, #8
    subs r0, #1
    bne value_loop
    bx lr
original:
    ldr r3, =A4_HANDLER
call_r3:
    bx r3

.thumb_func
ncmd_entry:
    ldr r0, =WINDOW
    ldr r1, [r0]
    ldr r3, =ARMED
    cmp r1, r3
    bne not_armed
    adds r0, #5
    bx r0
not_armed:
    movs r0, #0x2e
    ldr r3, =HOST_ERR
    bx r3
    .balign 4
    .ltorg
