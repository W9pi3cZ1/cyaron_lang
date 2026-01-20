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
                      // -> push(*int_ref(ptr))
OPCODE(OP_LOAD_ARR)   // LOAD_ARR(ptr: decl*)[idx: i32]
                      // -> push(*arr_ref(ptr, pop(idx)))
OPCODE(OP_STORE_INT)  // STORE_INT(ptr: decl*)[val: i32]
                      // -> *int_ref(ptr) = pop(val)
OPCODE(OP_STORE_ARR)  // STORE_ARR(ptr: decl*)[idx: i32, val: i32]
                      // -> *arr_ref(ptr, pop(idx)) = pop(val)
OPCODE(OP_SETI)       // SETI(ptr: decl*, const: i32)
                      // -> *int_ref(ptr) = const

OPCODE(OP_INCR) // INCR(const: i32)[num: i32]
                // -> push(pop(num)+const)
OPCODE(OP_INCI) // INCI(ptr: decl*, const: i32)
                // -> *int_ref(ptr) += const
// The below one is hard to impl ..
// OPCODE(OP_INCA)    // INCA(ptr: decl*, const: i32)[idx: i32]
//                    // -> *arr_ref(ptr, pop(idx)) += const
OPCODE(OP_CMUL)   // (Constant Mul) CMUL(const: i32)[a: i32]
                  // push(const*pop(a))
OPCODE(OP_BINADD) // BINADD[a1: i32, a2: i32]
                  // -> push(pop(a1)+pop(a2))

OPCODE(OP_PUT)  // PUT[num: i32]
                // -> print_num(pop(num))
OPCODE(OP_JMP)  // (Directly)JMP(offset: i16)
                // -> exec_ptr += offset
OPCODE(OP_CJMP) // CJMP(cmp_type: CmpType, offset: i16)[left: i32, right: i32]
                // -> if cmp(pop(left), pop(right)): jmp(offset)
OPCODE(OP_HALT) // HALT

//- Deprecated -//
// OPCODE(OP_TRIADD)  // TRIADD[a1: i32, a2: i32, a3: i32]
//                    // -> push(pop(a1)+pop(a2)+pop(a3))
// OPCODE(OP_QUADADD) // QUADADD[a1: i32, a2: i32, a3: i32, a4: i32]
//                    // -> push(pop(a1)+pop(a2)+pop(a3)+pop(a4))
// OPCODE(OP_ADDS)    // ADDS(term_cnts: i32)[a1: i32, a2: i32, ...]
//                    // -> push(pop(a1)+pop(a2)+pop(a3)+...)

#ifdef DEFAULT_DEF
#undef DEFAULT_DEF
#undef OPCODE
#endif