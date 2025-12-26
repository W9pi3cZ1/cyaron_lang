// OPCODE(arg)[arg0,arg1]
// `arg` is on TEXT, `arg0` is on STACK(0), `arg1` is on STACK(-1)
// push(sth): push sth to STACK, but will be aligned to 32-bits.

#ifndef OPCODE
#define DEFAULT_DEF
#define OPCODE(x)
#endif

OPCODE(OP_LOAD_CONST) // LOAD_CONST(const: i32)
                      // -> push(const)
OPCODE(OP_LOAD_INT)   // LOAD_INT(ptr: decl*)
                      // -> push(int_read(ptr))
OPCODE(OP_LOAD_ARR)   // LOAD_ARR(ptr: decl*)[idx: i32]
                      // -> push(arr_read(ptr, pop(idx)))
OPCODE(OP_STORE_INT)  // STORE_INT(ptr: decl*)[val: i32]
                      // -> int_write(ptr, pop(val))
OPCODE(OP_STORE_ARR)  // STORE_ARR(ptr: decl*)[idx: i32, val: i32]
                      // -> arr_write(decl, pop(idx), pop(val))

OPCODE(OP_JMP) // (Directly)JMP(offset: i16)
               // -> exec_ptr += offset
OPCODE(OP_JZ)  // (If !cond)JZ(offset: i16)[cond: i8]
               // -> exec_ptr += pop(cond)? 1 : offset
OPCODE(OP_JNZ) // (If cond)JNZ(offset: i16)[cond: i8]
               // -> exec_ptr += pop(cond)? offset : 1

OPCODE(OP_CMP) // CMP(cmp_type: CmpType)[left: i32, right: i32]
               // -> push(cmp(cmp_type, pop(left), pop(right))?1:0)

OPCODE(OP_INCR) // INCR[num: i32]
                // -> push(pop(num)+1)
OPCODE(OP_ADDS) // ADDS(term_cnts: i32)[a1: i32, a2: i32, ...]
                // -> push(pop(a1)+pop(a2)+pop(a3)+...)
OPCODE(OP_CMUL) // (Constant Mul) CMUL(const: i32)[a: i32]
                // push(const*pop(a))
OPCODE(OP_PUT)  // PUT[num: i32]
                // -> print_num(pop(num))
OPCODE(OP_HALT) // HALT

#ifdef DEFAULT_DEF
#undef DEFAULT_DEF
#undef OPCODE
#endif