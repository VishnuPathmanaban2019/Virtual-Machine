#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "lib/xalloc.h"
#include "lib/stack.h"
#include "lib/contracts.h"
#include "lib/c0v_stack.h"
#include "lib/c0vm.h"
#include "lib/c0vm_c0ffi.h"
#include "lib/c0vm_abort.h"

/* call stack frames */
typedef struct frame_info frame;
struct frame_info {
  c0v_stack_t S; /* Operand stack of C0 values */
  ubyte *P;      /* Function body */
  size_t pc;     /* Program counter */
  c0_value *V;   /* The local variables */
};

int execute(struct bc0_file *bc0) {
  REQUIRES(bc0 != NULL);

  /* Variables */
  c0v_stack_t S; /* Operand stack of C0 values */
  ubyte *P;      /* Array of bytes that make up the current function */
  size_t pc;     /* Current location within the current byte array P */
  c0_value *V;   /* Local variables (you won't need this till Task 2) */
  (void) V;

  /* The call stack, a generic stack that should contain pointers to frames */
  /* You won't need this until you implement functions. */
  gstack_t callStack;
  (void) callStack;
  
  //S, P, pc, and V initialization
  S = c0v_stack_new();
  P = (bc0->function_pool[0].code);
  pc = 0;
  V = xmalloc((bc0->function_pool[0].num_vars) * (sizeof(c0_value)));
  callStack = stack_new();

  while (true) {

#ifdef DEBUG
    /* You can add extra debugging information here */
    fprintf(stderr, "Opcode %x -- Stack size: %zu -- PC: %zu\n",
            P[pc], c0v_stack_size(S), pc);
#endif

    switch (P[pc]) {

    /* Additional stack operation: */

    case POP: {
      pc++;
      c0v_pop(S);
      break;
    }

    case DUP: {
      pc++;
      c0_value v = c0v_pop(S);
      c0v_push(S,v);
      c0v_push(S,v);
      break;
    }

    case SWAP: {
      pc++;
      c0_value a = c0v_pop(S);
      c0_value b = c0v_pop(S);
      c0v_push(S, a);
      c0v_push(S, b);
      break;
    }

    /* Returning from a function.
     * This currently has a memory leak! You will need to make a slight
     * change for the initial tasks to avoid leaking memory.  You will
     * need to be revise it further when you write INVOKESTATIC. */

    case RETURN: {
      int retval = val2int(c0v_pop(S));
      assert(c0v_stack_empty(S));
#ifdef DEBUG
      fprintf(stderr, "Returning %d from execute()\n", retval);
#endif
      // Free everything before returning from the execute function!
      c0v_stack_free(S);
      free(V);
      if(stack_size(callStack) != 0)
      {
        frame *stack_frame = ((frame*)(pop(callStack)));
        S = stack_frame->S;
        P = stack_frame->P;
        pc = stack_frame->pc;
        V = stack_frame->V;
        c0v_push(S, int2val(retval));
        free(stack_frame);
        break;
      }
      else
      {
        stack_free((callStack), (NULL));
        return retval;
      }
    }

    /* Arithmetic and Logical operations */

    case IADD: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      c0v_push(S, int2val(a + b));
      break;
    }

    case ISUB: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      c0v_push(S, int2val(b - a));
      break;
    }

    case IMUL: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      c0v_push(S, int2val(a * b));
      break;
    }

    case IDIV: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      if(a == -1 && b == INT_MIN)
      {
        c0_arith_error("ERROR: int_min divided by -1\n");
      }
      else if(a == 0)
      {
        c0_arith_error("ERROR: division by 0\n");
      }
      else 
      {
        c0v_push(S, int2val(b / a));
      }
      break;
    }

    case IREM: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      if(a == -1 && b == INT_MIN)
      {
        c0_arith_error("ERROR: int_min divided by -1\n");
      }
      else if(a == 0)
      {
        c0_arith_error("ERROR: division by 0\n");
      }
      else 
      {
        c0v_push(S, int2val(b % a));
      }
      break;
    }

    case IAND: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      c0v_push(S, int2val(a & b));
      break;
    }

    case IOR: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      c0v_push(S, int2val(a | b));
      break;
    }

    case IXOR: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      c0v_push(S, int2val(a ^ b));
      break;
    }

    case ISHL: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      if(a > 31)
      {
        c0_arith_error("ERROR: shift greater than 32\n");
      }
      else if(a < 0)
      {
        c0_arith_error("ERROR: negative shift\n");
      }
      else 
      {
        c0v_push(S, int2val(b << a));
      }
      break;
    }

    case ISHR: {
      pc++;
      int a = val2int(c0v_pop(S));
      int b = val2int(c0v_pop(S));
      if(a > 31)
      {
        c0_arith_error("ERROR: shift greater than 32\n");
      }
      else if(a < 0)
      {
        c0_arith_error("ERROR: negative shift\n");
      }
      else 
      {
        c0v_push(S, int2val(b >> a));
      }
      break;
    }

    /* Pushing constants */

    case BIPUSH: {
      pc++;
      int a = (int)((int8_t)(P[pc]));
      c0v_push(S, int2val(a));
      pc++;
      break;
    }

    case ILDC: {
      pc++;
      uint16_t x = (((uint16_t)(P[pc])) << 8);
      pc++;
      x |= ((uint16_t)(P[pc]));
      c0v_push(S, int2val((bc0->int_pool[x])));
      pc++;
      break;
    }

    case ALDC: {
      pc++;
      uint16_t x = (((uint16_t)(P[pc])) << 8);
      pc++;
      x |= ((uint16_t)(P[pc]));
      void *y = (void*)(&bc0->string_pool[x]);
      c0v_push(S, ptr2val(y));
      pc++;
      break;
    }

    case ACONST_NULL: {
      pc++;
      c0v_push(S, ptr2val((void*)(0)));
      break;
    }

    /* Operations on local variables */

    case VLOAD: {
      pc++;
      c0v_push(S, V[P[pc]]);
      pc++;
      break;
    }

    case VSTORE: {
      pc++;
      V[P[pc]] = (c0v_pop(S));
      pc++;
      break;
    }

    /* Assertions and errors */

    case ATHROW: {
      pc++;
      c0_user_error((char*)(val2ptr(c0v_pop(S))));
      break;
    }

    case ASSERT: {
      pc++;
      char* text = (char*)(val2ptr(c0v_pop(S)));
      if(val2int(c0v_pop(S)) == 0)
      {
        c0_assertion_failure(text);
      }
      break;
    }

    /* Control flow operations */

    case NOP: {
      pc = pc + 1;
      break;
    }

    case IF_CMPEQ: {
      pc = pc + 1;
      if(val_equal(c0v_pop(S), c0v_pop(S)) == false)
      {
        pc = pc + 2;
      }
      else
      {
        uint16_t item = ((uint16_t)P[pc]) << 8;
        pc = pc + 1;
        item |= ((uint16_t)(P[pc]));
        if((int16_t)(item) == 0)
        {
          pc = pc + 1;
        }
        else
        {
          pc = pc + (((int16_t)(item)) - 2);
        }
      }
      break;
    }

    case IF_CMPNE: {
      pc = pc + 1;
      if(val_equal(c0v_pop(S), c0v_pop(S)))
      {
        pc = pc + 2;
      }
      else
      {
        uint16_t item = ((uint16_t)P[pc]) << 8;
        pc = pc + 1;
        item |= ((uint16_t)(P[pc]));
        if((int16_t)(item) == 0)
        {
          pc = pc + 1;
        }
        else
        {
          pc = pc + (((int16_t)(item)) - 2);
        }
      }
      break;
    }

    case IF_ICMPLT: {
      pc = pc + 1;
      if(c0v_pop(S).payload.i <= c0v_pop(S).payload.i)
      {
        pc = pc + 2;
      }
      else
      {
        uint16_t item = ((uint16_t)P[pc]) << 8;
        pc = pc + 1;
        item |= ((uint16_t)(P[pc]));
        if((int16_t)(item) == 0)
        {
          pc = pc + 1;
        }
        else
        {
          pc = pc + (((int16_t)(item)) - 2);
        }
      }
      break;
    }

    case IF_ICMPGE: {
      pc = pc + 1;
      if(c0v_pop(S).payload.i > c0v_pop(S).payload.i)
      {
        pc = pc + 2;
      }
      else
      {
        uint16_t item = ((uint16_t)P[pc]) << 8;
        pc = pc + 1;
        item |= ((uint16_t)(P[pc]));
        if((int16_t)(item) == 0)
        {
          pc = pc + 1;
        }
        else
        {
          pc = pc + (((int16_t)(item)) - 2);
        }
      }
      break;
    }

    case IF_ICMPGT: {
      pc = pc + 1;
      if(c0v_pop(S).payload.i >= c0v_pop(S).payload.i)
      {
        pc = pc + 2;
      }
      else
      {
        uint16_t item = ((uint16_t)P[pc]) << 8;
        pc = pc + 1;
        item |= ((uint16_t)(P[pc]));
        if((int16_t)(item) == 0)
        {
          pc = pc + 1;
        }
        else
        {
          pc = pc + (((int16_t)(item)) - 2);
        }
      }
      break;
    }

    case IF_ICMPLE: {
      pc = pc + 1;
      if(c0v_pop(S).payload.i < c0v_pop(S).payload.i)
      {
        pc = pc + 2;
      }
      else
      {
        uint16_t item = ((uint16_t)P[pc]) << 8;
        pc = pc + 1;
        item |= ((uint16_t)(P[pc]));
        if((int16_t)(item) == 0)
        {
          pc = pc + 1;
        }
        else
        {
          pc = pc + (((int16_t)(item)) - 2);
        }
      }
      break;
    }

    case GOTO: {
      pc = pc + 1;
      uint16_t item = ((uint16_t)P[pc]) << 8;
      pc = pc + 1;
      item |= ((uint16_t)(P[pc]));
      if((int16_t)(item) == 0)
      {
        pc = pc + 1;
      }
      else
      {
        pc = pc + (((int16_t)(item)) - 2);
      }
      break;
    }

    /* Function call operations: */

    case INVOKESTATIC: {
      pc = pc + 1;
      uint16_t item = ((uint16_t)P[pc]) << 8;
      pc = pc + 1;
      item |= ((uint16_t)(P[pc]));
      frame* stack_frame = (xmalloc(sizeof(frame)));
      stack_frame->S = S;
      stack_frame->P = P;
      stack_frame->pc = pc + 1;
      stack_frame->V = V;
      push((callStack), ((void*)(stack_frame)));
      struct function_info fi = (bc0->function_pool[item]);
      V = xmalloc((sizeof(c0_value)) * ((int)(fi.num_vars)));
      for(int x = ((int)(fi.num_args) - 1); x > -1; x--)
      {
        V[x] = c0v_pop(S);
      }
      S = (c0v_stack_new());
      P = (fi.code);
      pc = 0;
      break;
    }

    case INVOKENATIVE: {
      pc = pc + 1;
      uint16_t item = ((uint16_t)P[pc]) << 8;
      pc = pc + 1;
      item |= ((uint16_t)(P[pc]));
      c0_value value_array [(int)(bc0->native_pool[item].num_args)];
      for(int x = ((int)(bc0->native_pool[item].num_args) - 1); x > -1; x--)
      {
        value_array[x] = c0v_pop(S);
      }
      c0v_push(S,(*native_function_table[bc0->native_pool[item].function_table_index])(value_array));
      pc = pc + 1;
      break;
    }

    /* Memory allocation operations: */

    case NEW: {
      pc = pc + 1;
      void *pointer = xmalloc((int)((int8_t)(P[pc])));
      c0v_push(S, ptr2val(pointer));
      pc = pc + 1;
      break;
    }

    case NEWARRAY: {
      pc = pc + 1;
      int x = val2int(c0v_pop(S));
      c0_array *new_array = xmalloc(sizeof(c0_array));
      new_array->elt_size = (int)((int8_t)P[pc]);
      new_array->elems = xcalloc(x, ((int)((int8_t)(P[pc]))));
      new_array->count = x;
      c0v_push(S, ptr2val((void*)(new_array)));
      pc = pc + 1;
      break;
    }

    case ARRAYLENGTH: {
      c0_array *selected = val2ptr(c0v_pop(S));
      if(selected != NULL)
      {
        c0v_push(S, int2val(selected->count));
        pc = pc + 1;
        break;
      }
      else
      {
        c0_memory_error("invalid array\n");
      }
    }

    /* Memory access operations: */

    case AADDF: {
      pc = pc + 1;
      char *selected = val2ptr(c0v_pop(S));
      if(selected != NULL)
      {
        c0v_push(S, ptr2val((selected)+((ubyte)(P[pc]))));
        pc = pc + 1;
        break;
      }
      else
      {
        c0_memory_error("invalid pointer\n");
      }
    }

    case AADDS: {
      int x = val2int(c0v_pop(S));
      c0_array *selected = val2ptr(c0v_pop(S));
      if(selected != NULL)
      {
        if((x >= selected->count) || (x <= -1))
        {
          c0_memory_error("out of bounds array access\n");
        }
        else
        {
          char *a = (selected->elems);
          void *b = ((a) + (selected->elt_size * x));
          c0v_push(S, ptr2val(b));
          pc = pc + 1;
          break;
        }
      }
      else
      {
        c0_memory_error("invalid pointer\n");
      }
    }

    case IMLOAD: {
      int *selected = val2ptr(c0v_pop(S));
      if(selected != NULL)
      {
        c0v_push(S, int2val(*selected));
        pc = pc + 1;
        break;
      }
      else
      {
        c0_memory_error("invalid pointer\n");
      }
    }

    case IMSTORE: {
      int x = val2int(c0v_pop(S));
      int *selected = (int*)(val2ptr(c0v_pop(S)));
      if(selected != NULL)
      {
        *selected = x;
        pc = pc + 1;
        break;
      }
      else
      {
        c0_memory_error("invalid pointer\n");
      }
    }

    case AMLOAD: {
      void **selected = (val2ptr(c0v_pop(S)));
      if(selected != NULL)
      {
        void *item = *selected;
        c0v_push(S, ptr2val(item));
        pc = pc + 1;
        break;
      }
      else
      {
        c0_memory_error("invalid pointer\n");
      }
    }

    case AMSTORE: {
      void *item = (val2ptr(c0v_pop(S)));
      void **selected = (val2ptr(c0v_pop(S)));
      if(selected != NULL)
      {
        *selected = item;
        pc = pc + 1;
        break;
      }
      else
      {
        c0_memory_error("invalid pointer\n");
      }
    }

    case CMLOAD: {
      char *selected = (val2ptr(c0v_pop(S)));
      if(selected != NULL)
      {
        int x = (int32_t)(*selected);
        c0v_push(S, int2val(x));
        pc = pc + 1;
        break;
      }
      else
      {
        c0_memory_error("invalid pointer\n");
      }
    }

    case CMSTORE: {
      pc = pc + 1;
      int x = (val2int(c0v_pop(S)));
      char* selected = (val2ptr(c0v_pop(S)));
      if(selected != NULL)
      {
        *selected = (x & 0x7f);
        break;
      }
      else
      {
        c0_memory_error("invalid pointer\n");
      }
    }

    default:
      fprintf(stderr, "invalid opcode: 0x%02x\n", P[pc]);
      abort();
    }
  }

  /* cannot get here from infinite loop */
  assert(false);
}
