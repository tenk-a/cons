        .code16
        .globl  _ia16_intrp
        .type   _ia16_intrp,@function

        .data
g_ia16intr_int_stub:
        .byte   0xCD, 0x00, 0xCB, 0x00      // INT imm8 ; RETF ; pad

        .text

// struct _INTR_REGS_W の フィールド・オフセット(byte)
O_AX    = 0
O_BX    = 2
O_CX    = 4
O_DX    = 6
O_SI    = 8
O_DI    = 10
O_CFLAG = 12
O_FLAGS = 14
O_ES    = 16
O_CS    = 18
O_SS    = 20
O_DS    = 22
O_BP    = 24

// 引数
A_INTR_NO   = 4
A_IREGS_OFS = 6
A_IREGS_SEG = 8
A_OREGS_OFS = 10
A_OREGS_SEG = 12

_ia16_intrp:
        push    %bp
        mov     %sp, %bp

        push    %es
        push    %di
        push    %si
        push    %bx
        push    %cx
        push    %dx

        push    %bp     // +18 saved_BP
        push    %ds     // +16 saved_DS

        //mov   A_INTR_NO(%bp), %ah
        //movb  $0xCD,%al
        //mov   %ax, g_ia16intr_int_stub
        //mov   $0xCB,%al
        //movb  %al, g_ia16intr_int_stub+2
        movb    A_INTR_NO(%bp), %al
        movb    %al, g_ia16intr_int_stub+1

        /* ---- iregs: es:di = (seg:ofs) ---- */
        movw    A_IREGS_SEG(%bp), %ax
        mov     %ax, %es
        movw    A_IREGS_OFS(%bp), %di

        push    %cs                             /* stub から戻って来るアドレスのセグメント */
        movw    $1f, %ax                        // ax に 1: のアドレスを入れる.
        push    %ax                             /* stub から戻って来るアドレスのオフセット */

        push    %ds                             /* スタブ アドレスのセグメント */
        movw    $g_ia16intr_int_stub, %ax
        pushw   %ax                             /* スタブ アドレスのオフセット */

        movw    %es:O_AX(%di), %ax
        movw    %es:O_BX(%di), %bx
        movw    %es:O_CX(%di), %cx
        movw    %es:O_DX(%di), %dx
        movw    %es:O_SI(%di), %si
        movw    %es:O_BP(%di), %bp

        pushw   %es:O_DS(%di)
        pushw   %es:O_ES(%di)
        movw    %es:O_DI(%di), %di
        pop     %es
        pop     %ds

        retf    // call g_ia16intr_int_stub.
1:              // 復帰アドレス.

        // int ?? の結果のレジスタを待避.
        pushf           // +14
        push    %ds     // +12
        push    %es     // +10
        push    %cs     // +8
        push    %ss     // +6
        push    %di     // +4
        push    %ax     // +2
        push    %bp     // +0

        mov     %sp,%bp                 // bp = sp
        mov     16(%bp),%ax             // ax = saved_DS
        mov     %ax,%ds                 // ds = ax
        mov     18(%bp),%ax             // ax = saved_BP
        mov     %ax,%bp                 // bp = ax

        // ES:DI = (A_OREGS_SEG:A_OREGS_OFS)
        mov     A_OREGS_SEG(%bp), %ax   // ax = oregs.seg
        mov     %ax, %es                // es = ax
        mov     A_OREGS_OFS(%bp), %di   // di = oregs.ofs

        popw    %es:O_BP(%di)
        popw    %es:O_AX(%di)
        popw    %es:O_DI(%di)
        popw    %es:O_SS(%di)
        popw    %es:O_CS(%di)
        popw    %es:O_ES(%di)
        popw    %es:O_DS(%di)
        popw    %es:O_FLAGS(%di)

        movw    %bx, %es:O_BX(%di)
        movw    %cx, %es:O_CX(%di)
        movw    %dx, %es:O_DX(%di)
        movw    %si, %es:O_SI(%di)

        movw    %es:O_FLAGS(%di), %ax   // ax = es:di->o_flag
        and     $1, %ax                 // ax &= 1
        movw    %ax, %es:O_CFLAG(%di)   // es:di->o_cflg = ax

        add     $4, %sp                 // saved_DS,saved_BP 破棄.

        // 待避レジスタ復帰.
        pop     %dx
        pop     %cx
        pop     %bx
        pop     %si
        pop     %di
        pop     %es
        pop     %bp
        ret
