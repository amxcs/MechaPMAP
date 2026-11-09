.syntax unified
.cpu arm7tdmi
.thumb
.section .text

.include "wprof.inc"

.global window_start
window_start:
    .word 0
    b n_entry
    nop
    b call_entry
    nop

.thumb_func
call_entry:
    push {r4, r5, lr}
    lsrs r5, r0, #16
    cmp r0, #0x20
    blo old_ops
.if WALK
    cmp r0, #0x27
    bls walk_ops
    lsls r1, r0, #16
    lsrs r1, r1, #24
    cmp r1, #0
    bne bad_op
    lsls r1, r0, #24
    lsrs r1, r1, #24
    cmp r1, #0x20
    blo bad_op
    cmp r1, #0x27
    bhi bad_op
    adds r0, r1, #0
walk_ops:
    cmp r0, #0x24
    beq w_fe
    cmp r0, #0x25
    beq w_te
    cmp r0, #0x26
    beq w_f0
    cmp r0, #0x27
    beq w_f4c
    cmp r0, #0x21
    beq w_read
    cmp r0, #0x22
    beq w_reg16
    cmp r0, #0x23
    beq w_stock

    lsls r0, r5, #23
    asrs r0, r0, #23
    adds r4, r0, #0
    ldr r3, =0x0002003d
    bl call_r3
    adds r0, r4, #0
    b call_out
w_read:
    ldr r3, =0x000200e5
    bl call_r3
    b call_out
w_reg16:
    cmp r5, #0x7f
    bhi bad_op
    adds r1, r5, #0
    movs r0, #0x16
    ldr r3, =0x00007e7d
    bl call_r3
    adds r0, r5, #0
    b call_out
w_stock:
    ldr r1, =0x02000388
    ldrb r4, [r1]
    ldr r3, =0x0001d851
    cmp r4, #0
    beq w_stock_go
    ldr r3, =0x0001d895
w_stock_go:
    bl call_r3
    adds r0, r4, #0
    b call_out

w_fe:
    ldr r0, =0x34130000
    b w_gain
w_te:
    ldr r0, =0x34230000
w_gain:
    lsls r4, r5, #24
    lsrs r4, r4, #24
    lsls r1, r4, #8
    orrs r0, r1
    ldr r3, =0x00007dc5
    bl call_r3
    adds r0, r4, #0
    b call_out
w_f0:
    lsls r0, r5, #23
    asrs r0, r0, #23
    adds r4, r0, #0
    ldr r3, =0x00020061
    bl call_r3
    adds r0, r4, #0
    b call_out
w_f4c:
    lsls r4, r5, #23
    asrs r4, r4, #23
    lsls r0, r5, #9
    ldr r1, =0x0003fe00
    ands r0, r1
    ldr r1, =0x34f40000
    orrs r0, r1
    ldr r3, =0x00007dc5
    bl call_r3
    adds r0, r4, #0
    b call_out
.else
    b bad_op
.endif
old_ops:
    cmp r0, #0x11
    blo direct
    cmp r0, #0x17
    bhi bad_op
    ldr r1, =JOB
    str r0, [r1]
    b call_out
direct:
    subs r0, #1
    cmp r0, #13
    bhi bad_op
    lsls r0, r0, #3
    adr r4, ops
    adds r4, r4, r0
    ldr r1, =PMAP_PARAM
    ldrb r2, [r4, #4]
    strb r2, [r1]
    ldrb r2, [r4, #5]
    strb r2, [r1, #1]
    ldrb r2, [r4, #6]
    strb r2, [r1, #2]
    ldrb r2, [r4, #7]
    strb r2, [r1, #3]
    bl preset_reply
    ldr r3, [r4]
    bl call_r3
    bl copy_reply
    b call_out
bad_op:
    movs r0, #0
    mvns r0, r0
call_out:
    pop {r4, r5}
    pop {r3}
    bx r3

preset_reply:
    ldr r1, =PMAP_REPLY
    movs r2, #0
    mvns r2, r2
    str r2, [r1]
    str r2, [r1, #4]
    bx lr

copy_reply:
    ldr r1, =PMAP_REPLY
    ldr r2, =RESULT
    movs r3, #0
copy_loop:
    ldrb r0, [r1, r3]
    strb r0, [r2, r3]
    adds r3, #1
    cmp r3, #8
    bne copy_loop
    ldr r0, [r2]
    bx lr

call_r3:
    bx r3

.thumb_func
n_entry:
    push {r4, r5, r6, r7, lr}
    sub sp, #12
    ldr r4, =JOB
    ldr r5, [r4]
    movs r0, #0
    str r0, [r4]
    movs r6, #0x2d
    cmp r5, #0x11
    blo n_error
    cmp r5, #0x17
    bhi n_error
    ldr r1, =LID
.if LID_BYTE

    ldrb r0, [r1]
    movs r6, #0x24
    cmp r0, #0
    bne n_error
.else
    ldr r0, [r1]
    movs r6, #0x24
    lsls r0, r0, #25
    bpl n_error
.endif

    cmp r5, #0x15
    blo 1f
    b n_c97
1:

    ldr r0, =0x00020005
    movs r1, #0
    cmp r5, #0x12
    beq have_req
    cmp r5, #0x14
    beq have_req
    ldr r0, =0x0002000c
    cmp r5, #0x11
    beq have_req
    ldr r0, =0x01020000
    ldr r1, =0x0003f000
have_req:
    str r0, [sp, #4]
    str r1, [sp, #8]
    cmp r5, #0x12
    bne not_ca3
    ldr r1, =CA_MODE
    movs r0, #0
    strb r0, [r1]
    ldr r1, =CA_FLAG
    movs r0, #1
    strb r0, [r1]
    ldr r1, =PMAP_PARAM
    strb r0, [r1]
not_ca3:
    cmp r5, #0x14
    bne not_ca5
    ldr r1, =CA_MODE
    movs r0, #5
    strb r0, [r1]
    ldr r1, =PMAP_PARAM
    strb r0, [r1, #1]
    movs r0, #1
    strb r0, [r1]
    ldr r1, =CA_FLAG
    strb r0, [r1]
    subs r1, #1
    strb r0, [r1]
not_ca5:
    cmp r5, #0x11
    bne not_c16
    ldr r1, =DISC
    movs r0, #0
    str r0, [r1]
not_c16:
    bl preset_reply

    movs r0, #0x0b
    mov r1, sp
    strb r0, [r1]
    movs r2, #1
    movs r3, #0
    mvns r3, r3
    movs r0, #2
    ldr r7, =K_POST
    bl call_r7

    add r1, sp, #4
    movs r2, #8
    movs r3, #0
    mvns r3, r3
    movs r0, #1
    ldr r7, =K_POST
    bl call_r7

    mov r0, sp
    movs r1, #2
    movs r2, #0
    mvns r2, r2
    ldr r7, =K_WAIT
    bl call_r7
    ldr r0, [sp]
    cmp r5, #0x12
    beq tail_ca3
    cmp r5, #0x14
    beq tail_ca5
    cmp r5, #0x11
    beq tail_c16

    movs r3, #4
    ldrsb r0, [r0, r3]
    ldr r1, =PMAP_REPLY
    movs r2, #0
    strb r2, [r1]
    strb r2, [r1, #1]
    cmp r0, #0
    bne n_done
    movs r2, #1
    strb r2, [r1]
    strb r2, [r1, #1]
    movs r2, #0x22
    strb r2, [r1, #2]
    b n_done
tail_ca3:
    movs r3, #4
    ldrsb r0, [r0, r3]
    cmp r0, #0
    beq ca3_go
    movs r0, #1
ca3_go:
    bl enter_ca3
    b n_done
tail_ca5:
    movs r3, #4
    ldrsb r0, [r0, r3]
    cmp r0, #0
    beq ca5_go
    movs r0, #1
ca5_go:
    bl enter_ca5
    b n_done
tail_c16:
    bl enter_c16
n_done:
    bl copy_reply
    ldr r7, =H_DONE
    bl call_r7
    b n_out
n_error:
    adds r0, r6, #0
    ldr r7, =H_ERR
    bl call_r7
n_out:
    add sp, #12
    pop {r4, r5, r6, r7}
    pop {r3}
    bx r3

n_c97:
    bl preset_reply
    ldr r1, =PMAP_PARAM
    movs r0, #1
    cmp r5, #0x15
    beq n_c97_arg
    movs r0, #3
    cmp r5, #0x16
    beq n_c97_arg
    movs r0, #0
n_c97_arg:
    strb r0, [r1]
    ldr r3, =H_C97
    bl call_r3
    b n_done

enter_ca3:
    push {lr}
    ldr r3, =T_CA3
    bx r3

enter_ca5:
    push {lr}
    ldr r3, =T_CA5
    bx r3
enter_c16:
    push {r0, lr}
    ldr r3, =T_C16
    bx r3
call_r7:
    bx r7

    .balign 4

ops:
    .word H_C17; .byte 0x00, 0x00, 0x00, 0x00
    .word H_CD3; .byte 0x13, 0x00, 0x00, 0x00
    .word H_CD3; .byte 0x23, 0x00, 0x00, 0x00
    .word H_CA7; .byte 0x00, 0x00, 0x00, 0x00
    .word H_CE9; .byte 0x01, 0x00, 0x00, 0x00
    .word H_CD1; .byte 0xd6, 0xd8, 0x00, 0x00
    .word H_CD1; .byte 0xd6, 0xde, 0x00, 0x00
    .word H_C15; .byte 0x00, 0x00, 0x00, 0x00
    .word H_C44; .byte 0x00, 0x00, 0x00, 0x00
    .word H_C22; .byte 0x00, 0x00, 0x00, 0x00
    .word H_C10; .byte 0x00, 0x00, 0x00, 0x00
    .word H_C14; .byte 0x00, 0x00, 0x00, 0x00
    .word H_C11; .byte 0x00, 0x00, 0x00, 0x00
    .word H_CE2; .byte 0x06, 0x00, 0x00, 0x00
    .ltorg

    .balign 4
.global JOB
.global RESULT
JOB:    .word 0
RESULT: .word 0, 0
.global window_end
window_end:
