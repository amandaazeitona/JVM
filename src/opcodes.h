#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

enum opcodes {
    opcode_nop = 0x00, opcode_aconst_null, opcode_iconst_m1,
    opcode_iconst_0, opcode_iconst_1, opcode_iconst_2, opcode_iconst_3, opcode_iconst_4, opcode_iconst_5,
    opcode_lconst_0, opcode_lconst_1, opcode_fconst_0, opcode_fconst_1, opcode_fconst_2, opcode_dconst_0,
    opcode_dconst_1, opcode_bipush, opcode_sipush, opcode_ldc, opcode_ldc_w, opcode_ldc2_w, opcode_iload,
    opcode_lload, opcode_fload, opcode_dload, opcode_aload, opcode_iload_0, opcode_iload_1, opcode_iload_2,
    opcode_iload_3, opcode_lload_0, opcode_lload_1, opcode_lload_2, opcode_lload_3, opcode_fload_0, opcode_fload_1,
    opcode_fload_2, opcode_fload_3, opcode_dload_0, opcode_dload_1, opcode_dload_2, opcode_dload_3, opcode_aload_0,
    opcode_aload_1, opcode_aload_2, opcode_aload_3, opcode_iaload, opcode_laload, opcode_faload, opcode_daload,
    opcode_aaload, opcode_baload, opcode_caload, opcode_saload, opcode_istore, opcode_lstore, opcode_fstore,
    opcode_dstore, opcode_astore, opcode_istore_0, opcode_istore_1, opcode_istore_2, opcode_istore_3, opcode_lstore_0,
    opcode_lstore_1, opcode_lstore_2, opcode_lstore_3, opcode_fstore_0, opcode_fstore_1, opcode_fstore_2,
    opcode_fstore_3, opcode_dstore_0, opcode_dstore_1, opcode_dstore_2, opcode_dstore_3, opcode_astore_0,
    opcode_astore_1, opcode_astore_2, opcode_astore_3, opcode_iastore, opcode_lastore, opcode_fastore,
    opcode_dastore, opcode_aastore, opcode_bastore, opcode_castore, opcode_sastore, opcode_pop, opcode_pop2,
    opcode_dup, opcode_dup_x1, opcode_dup_x2, opcode_dup2, opcode_dup2_x1, opcode_dup2_x2, opcode_swap,
    opcode_iadd, opcode_ladd, opcode_fadd, opcode_dadd, opcode_isub, opcode_lsub, opcode_fsub, opcode_dsub,
    opcode_imul, opcode_lmul, opcode_fmul, opcode_dmul, opcode_idiv, opcode_ldiv, opcode_fdiv, opcode_ddiv,
    opcode_irem, opcode_lrem, opcode_frem, opcode_drem, opcode_ineg, opcode_lneg, opcode_fneg, opcode_dneg,
    opcode_ishl, opcode_lshl, opcode_ishr, opcode_lshr, opcode_iushr, opcode_lushr, opcode_iand, opcode_land,
    opcode_ior = 0x80, opcode_lor, opcode_ixor, opcode_lxor, opcode_iinc, opcode_i2l, opcode_i2f, opcode_i2d,
    opcode_l2i, opcode_l2f, opcode_l2d, opcode_f2i, opcode_f2l, opcode_f2d, opcode_d2i, opcode_d2l, opcode_d2f,
    opcode_i2b, opcode_i2c, opcode_i2s, opcode_lcmp, opcode_fcmpl, opcode_fcmpg, opcode_dcmpl, opcode_dcmpg,
    opcode_ifeq, opcode_ifne, opcode_iflt, opcode_ifge, opcode_ifgt, opcode_ifle, opcode_if_icmpeq, opcode_if_icmpne,
    opcode_if_icmplt, opcode_if_icmpge, opcode_if_icmpgt, opcode_if_icmple, opcode_if_acmpeq, opcode_if_acmpne,
    opcode_goto, opcode_jsr, opcode_ret, opcode_tableswitch, opcode_lookupswitch, opcode_ireturn, opcode_lreturn,
    opcode_freturn, opcode_dreturn, opcode_areturn, opcode_return, opcode_getstatic, opcode_putstatic,
    opcode_getfield, opcode_putfield, opcode_invokevirtual, opcode_invokespecial, opcode_invokestatic, opcode_invokeinterface,
    opcode_invokedynamic, opcode_new, opcode_newarray, opcode_anewarray, opcode_arraylength, opcode_athrow,
    opcode_checkcast, opcode_instanceof, opcode_monitorenter, opcode_monitorexit, opcode_wide, opcode_multianewarray,
    opcode_ifnull, opcode_ifnonnull, opcode_goto_w,opcode_jsr_w,
    opcode_breakpoint = 0xCA, opcode_impdep1 = 0xFE, opcode_impdep2
};

typedef enum opcode_newarray_type {
    T_BOOLEAN = 4,
    T_CHAR,
    T_FLOAT,
    T_DOUBLE,
    T_BYTE,
    T_SHORT,
    T_INT,
    T_LONG
} opcode_newarray_type;

const char *decode_opcode_newarraytype(uint8_t);
const char *get_opcode_mnemonic(uint8_t);
#endif
