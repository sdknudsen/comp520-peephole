/******************************
 * Group comp520-2016-14
 * ============================
 * - Alexandre St-Louis Fortier
 * - Stefan Knudsen
 * - Cheuk Chuen Siow
 ******************************/

/**** Helper functions ****/

enum iMATH {
  IADD = 0x1,
  ISUB = 0x2,
  IMUL = 0x4,
  IDIV = 0x8
};

int is_iMath(CODE *c, unsigned int ops)
{
  if (ops & IADD) { return is_iadd(c); }
  if (ops & ISUB) { return is_isub(c); }
  if (ops & IMUL) { return is_imul(c); }
  if (ops & IDIV) { return is_idiv(c); }
  return 0;
}

/* Make sure the nodes are visited before being replaced 
   for correct stack height calculation */
void visit_nodes(CODE *c, int lines)
{ int i; CODE *c1;
  for( i = 0; i < lines; i = i + 1 ) {
    c1 = nextby(c, i);
    if (c1!=NULL && !c1->visited) {
      c1->visited = 1;
    }
  }
}

int only_affect_stack(CODE *c)
{
  if (c == NULL) return 0;
  switch (c->kind)
  {
  case nopCK:
  case instanceofCK:
  case i2cCK:
  case inegCK:
  case getfieldCK:	
  case swapCK:
  case iloadCK:
  case aloadCK:
  case ldc_intCK:
  case ldc_stringCK:
  case aconst_nullCK:
  case dupCK:
  case popCK:	
  case iremCK:
  case isubCK:
  case idivCK:
  case iaddCK:
  case imulCK:
    return 1;
  case iincCK:
  case checkcastCK:
  case labelCK:
  case newCK:
  case istoreCK:
  case astoreCK:
  case putfieldCK:
  case invokevirtualCK:
  case invokenonvirtualCK:
  case gotoCK:
  case ifeqCK:
  case ifneCK:
  case ifnullCK:
  case ifnonnullCK:
  case if_icmpeqCK:
  case if_icmpneCK:
  case if_icmpgtCK:
  case if_icmpltCK:
  case if_icmpleCK:
  case if_icmpgeCK:
  case if_acmpeqCK:
  case if_acmpneCK:
  case returnCK:
  case areturnCK:
  case ireturnCK:
  default:
    return 0;
  }
}

/*
 * JOOS is Copyright (C) 1997 Laurie Hendren & Michael I. Schwartzbach
 *
 * Reproduction of all or part of this software is permitted for
 * educational or research use on condition that this copyright notice is
 * included in any copy. This software comes with no warranty of any
 * kind. In no event will the authors be liable for any damages resulting from
 * use of this software.
 *
 * email: hendren@cs.mcgill.ca, mis@brics.dk
 */

/* iload x        iload x        iload x
 * ldc 0          ldc 1          ldc 2
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */
int simplify_multiplication_right(CODE **c)
{ int x,k;
  if (is_iload(*c,&x) && 
      is_ldc_int(next(*c),&k) && 
      is_imul(next(next(*c)))) {
     if (k==0) return replace(c,3,makeCODEldc_int(0,NULL));
     else if (k==1) return replace(c,3,makeCODEiload(x,NULL));
     else if (k==2) return replace(c,3,makeCODEiload(x,
                                       makeCODEdup(
                                       makeCODEiadd(NULL))));
     return 0;
  }
  return 0;
}

/* dup
 * astore x
 * pop
 * -------->
 * astore x
 */
int simplify_astore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_astore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEastore(x,NULL));
  }
  return 0;
}

/* iload x
 * ldc k   (0<=k<=127)
 * iadd
 * istore x
 * --------->
 * iinc x k
 */ 
int positive_increment(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_iadd(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      x==y && 0<=k && k<=127) {
     return replace(c,4,makeCODEiinc(x,k,NULL));
  }
  return 0;
}

/* goto L1
 * ...
 * L1:
 * goto L2
 * ...
 * L2:
 * --------->
 * goto L2
 * ...
 * L1:    (reference count reduced by 1)
 * goto L2
 * ...
 * L2:    (reference count increased by 1)  
 */
int simplify_goto_goto(CODE **c)
{ int l1,l2;
  if (is_goto(*c,&l1) && is_goto(next(destination(l1)),&l2) && l1>l2) {
     droplabel(l1);
     copylabel(l2);
     return replace(c,1,makeCODEgoto(l2,NULL));
  }
  return 0;
}



/*******************************************
 * Group comp520-2016-14's peephole patterns
 *******************************************/

/* 
 * ldc 0          ldc 1          ldc 2
 * iload x        iload x        iload x
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */
/* Soundness: each of these patterns is an arithmetic invariant */
/* the first two strictly decrease the number of instructions */
/* the third has the same number of instructions, but strictly decreases the number of bytes */
int simplify_multiplication_left(CODE **c)
{ int x,k;
  if (is_ldc_int(*c,&k) && 
      is_iload(next(*c),&x) && 
      is_imul(nextby(*c,2))) {
     if (k==0) return replace(c,3,makeCODEldc_int(0,NULL));
     else if (k==1) return replace(c,3,makeCODEiload(x,NULL));
     else if (k==2) return replace(c,3,makeCODEiload(x,
                                       makeCODEdup(
                                       makeCODEiadd(NULL))));
     return 0;
  }
  return 0;
}

/* dup
 * istore x
 * pop
 * -------->
 * istore x
 */
/* Soundness: istore already pops the stack */
/* the number of instructions strictly decreases */
int simplify_istore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_istore(next(*c),&x) &&
      is_pop(nextby(*c,2))) {
     return replace(c,3,makeCODEistore(x,NULL));
  }
  return 0;
}

/* aload x
 * aload x
 * --------->
 * aload x
 * dup
 */
/* Soundness: dup does the same thing as a second aload in  less memory (assuming what's to be loaded is on the top of the stack) */
/* the number of bytes strictly decreases */
int simplify_aload(CODE **c)
{ int x1,x2;
  if (is_aload(*c,&x1) &&
      is_aload(next(*c),&x2) &&
      x1 == x2) {
    return replace(c,2,makeCODEaload(x1,
                       makeCODEdup(NULL)));
  }
  return 0;
}

/*
 * astore x
 * aload x
 * --------->
 * dup
 * astore x
 */
/* Soundness: a load isn't necessary if the object to be loaded is already on the stack (we counter the pop from the store with a dup) */
/* the number of bytes strictly decreases */
int optimize_astore(CODE **c)
{ int x, y;
  if (!is_astore(*c, &x))      { return 0; }
  if (!is_aload(next(*c), &y)) { return 0; }
  if (x == y)
    return replace(c, 2, makeCODEdup(makeCODEastore(x,NULL)));
  return 0;
}

/*
 * istore x
 * iload x
 * --------->
 * dup
 * istore x
 */
/* Soundness: same reasoning as last pattern, but with integers */
/* the number of bytes strictly decreases */
int optimize_istore(CODE **c)
{ int x, y;
  if (!is_istore(*c, &x))      { return 0; }
  if (!is_iload(next(*c), &y)) { return 0; }
  if (x == y)
    return replace(c, 2, makeCODEdup(makeCODEistore(x,NULL)));
  return 0;
}

/* iload x
 * iload x
 * --------->
 * iload x
 * dup
 */
/* Soundness: a dup is fewer bytes, and results in the same thing thing as a repeated load */
int simplify_iload(CODE **c)
{ int x1,x2;
  if (is_iload(*c,&x1) &&
      is_iload(next(*c),&x2) &&
      x1 == x2) {
    return replace(c,2,makeCODEiload(x1,
                       makeCODEdup(NULL)));
  }
  return 0;
}

/* aload x
 * getfield
 * aload x
 * swap
 * -------->
 * aload x
 * dup
 * getfield
 */
/* Soundness: a dup is fewer bytes, and results in the same thing thing as a repeated load; the swap isn't needed if we swap the instructions here */
int simplify_aload_getfield_aload_swap(CODE **c)
{ int x; char *y; int z;
  if (is_aload(*c,&x) &&
      is_getfield(next(*c),&y) &&
      is_aload(next(next(*c)),&z) &&
      is_swap(next(next(next(*c)))) &&
      x == z) {
     return replace(c,4,makeCODEaload(x,
                        makeCODEdup(
                        makeCODEgetfield(y,NULL))));
  }
  return 0;
}

/* /\* goto L1 */
/*  * ... */
/*  * L1: */
/*  * L2: */
/*  * ---------> */
/*  * goto L2 */
/*  * ... */
/*  * L1: */
/*  * L2: */
/*  *\/  */
/* int remove_extra_labels_goto(CODE **c) */
/* { int l1,l2; */
/*   if (is_goto(*c,&l1) && is_label(next(destination(l1)),&l2)) { */
/*   /\* if (is_goto(*c,&l1) && is_label(next(destination(l1)),&l2) && l1>l2) { *\/ */
/*      droplabel(l1); */
/*      copylabel(l2); */
/*      return replace(c,1,makeCODEgoto(l2,NULL)); */
/*   } */
/*   return 0; */
/* } */
/* int remove_extra_labels_ifeq(CODE **c) */
/* { int l1,l2; */
/*   if (is_ifeq(*c,&l1) && is_label(next(destination(l1)),&l2)) { */
/*      droplabel(l1); */
/*      copylabel(l2); */
/*      return replace(c,1,makeCODEifeq(l2,NULL)); */
/*   } */
/*   return 0; */
/* } */
/* int remove_extra_labels_icmpeq(CODE **c) */
/* { int l1,l2; */
/*   if (is_if_icmpeq(*c,&l1) && is_label(next(destination(l1)),&l2)) { */
/*      droplabel(l1); */
/*      copylabel(l2); */
/*      return replace(c,1,makeCODEif_icmpeq(l2,NULL)); */
/*   } */
/*   return 0; */
/* } */
/* int remove_extra_labels_icmpne(CODE **c) */
/* { int l1,l2; */
/*   if (is_if_icmpne(*c,&l1) && is_label(next(destination(l1)),&l2)) { */
/*      droplabel(l1); */
/*      copylabel(l2); */
/*      return replace(c,1,makeCODEif_icmpne(l2,NULL)); */
/*   } */
/*   return 0; */
/* } */

/* dup
 * aload x
 * swap
 * putfield k
 * pop
 * -------->
 * aload x
 * swap
 * putfield k
 */
/* Soundness (since the first and last lines are the same):
 * [...:v]
 * [...:v:v]
 * [...:v:v:local[x]]
 * [...:v:local[x]:v]
 * [...:v], local[x].f=v
 * [...], local[x].f=v
 * -------->
 * [...:v]
 * [...:v:local[x]]
 * [...:local[x]:v]
 * [...], local[x].f=v
 */
int simplify_aload_swap_putfield(CODE **c)
{ int x; char *k;
  if (is_dup(*c) &&
      is_aload(next(*c),&x) &&
      is_swap(nextby(*c,2)) &&
      is_putfield(nextby(*c,3),&k) &&
      is_pop(nextby(*c,4))) {
     return replace(c,5,makeCODEaload(x,
                        makeCODEswap(
                        makeCODEputfield(k,NULL))));
  }
  return 0;
}

/* ldc x
 * ldc y
 * iadd / isub / imul / idiv / irem
 * ------>
 * ldc x (+|-|*|/|%) y
 */
int simplify_constant_op(CODE **c)
{ int x,y,k;
  int num_neg = 0;
  int xneg, yneg;

  if (!is_ldc_int(*c, &x))
    return 0;
  if ((xneg = is_ineg(next(*c))))
    num_neg++;

  if (!is_ldc_int(nextby(*c, 1 + num_neg), &y))
    return 0;
  if ((yneg = is_ineg(nextby(*c, 2 + num_neg))))
    num_neg++;

  if      (is_iadd(nextby(*c,2+num_neg))) { k = (xneg?-x:x)+(yneg?-y:y); }
  else if (is_isub(nextby(*c,2+num_neg))) { k = (xneg?-x:x)-(yneg?-y:y); }
  else if (is_imul(nextby(*c,2+num_neg))) { k = (xneg?-x:x)*(yneg?-y:y); }
  else if (y == 0) return 0; /* Division or Modulo by zero should fail at runtime */
                             /* and not crash the compiler. */
  else if (is_idiv(nextby(*c,2+num_neg))) { k = (xneg?-x:x)/(yneg?-y:y); }
  else if (is_irem(nextby(*c,2+num_neg))) { k = (xneg?-x:x)%(yneg?-y:y); }
  else return 0; /* Not a valid pattern */

  if (k >= 0) {
    return replace(c,3+num_neg,makeCODEldc_int(k,NULL));
  } else {
    /* Should take less instructions even if we add `ineg`. */
    return replace(c,3+num_neg,makeCODEldc_int(0-k,
                               makeCODEineg(NULL)));
  }
}

/*
 * return    
 * L:
 * return
 * ------->
 * L:
 * return
 */
/* Soundness: the label won't change any behaviour */
int remove_superfluous_return(CODE** c)
{ int L;
  CODE *c1 = next(*c);
  CODE *c2 = next(c1);
  if (!is_return(*c) || !is_ireturn(*c) || !is_areturn(*c)) { return 0; }
  if (!is_label(c1, &L))                                    { return 0; }
  if (!is_return(c2) || !is_ireturn(c2) || !is_areturn(c2)) { return 0; }
  return replace(c, 1, NULL);
}

/* iload x        iload x       ldc 0
 * ldc 0          ldc 1         load x
 * iadd / isub    idiv          iadd
 * ------>        ------>       ------>
 * iload x        iload x       iload x
 */
/* Soundness: these are arithmetic invariants */
int simplify_trivial_op(CODE **c)
{ int x,k;
  CODE *c1, *c2;
  c1 = next(*c);
  c2 = nextby(*c, 2);
  if ((is_iload(*c,&x) &&
       is_ldc_int(c1,&k) &&
       ((k == 0 && is_iMath(c2, IADD|ISUB)) ||
        (k == 1 && is_iMath(c2, IDIV)))) ||
        (is_ldc_int(*c,&k) &&
         is_iload(c1,&x) &&
         ((k == 0 && is_iadd(c2))))) {
    return replace(c,3,makeCODEiload(x,NULL));
  }
  return 0;
}

/* swap
 * swap
 * --------->
 * 
 *
 * Soundness (the first and last lines are the same):
 * [...:x:y]
 * [...:y:x]
 * [...:x:y]
 * -------->
 * [...:x:y]
 */
int remove_2_swap(CODE **c)
{ 
  if (is_swap(*c) &&
      is_swap(next(*c))) {
    return replace_modified(c,2,NULL);
  }
  return 0;
}

/* aload x
 * astore x
 * --------->
 * 
 *
 * Soundness (the first and last lines are the same):
 * [...]
 * [...:local[x]]
 * [...]	local[k]=local[k]
 * -------->
 * [...]	local[k]=local[k] (always true)
 */
int remove_aload_astore(CODE **c)
{ int x1,x2;
  if (is_aload(*c,&x1) &&
      is_astore(next(*c),&x2) &&
      x1 == x2)
  {
    return replace_modified(c,2,NULL);
  }
  return 0;
}

/* iload x
 * istore x
 * --------->
 * 
 *
 * Soundness (same as for aload, but with ints):
 * [...]
 * [...:local[x]]
 * [...]	local[k]=local[k]
 * -------->
 * [...]	local[k]=local[k] (always true)
 */
int remove_iload_istore(CODE **c)
{ int x1,x2;
  if (is_iload(*c,&x1) &&
      is_istore(next(*c),&x2) &&
      x1 == x2) {
    return replace_modified(c,2,NULL);
  }
  return 0;
}


/*
 * Dead labels serve no purpose.
 * They don't take space but they usually break patterns.
 * So we kill them.
 *
 * L:    (with no incoming edges)
 * --------->
 * 
 * Soundness : if a label has no incoming edges, the program
 * does not use it and won't use it in the future
 */
int remove_deadlabel(CODE **c)
{ int l;
  if(is_label(*c,&l) && deadlabel(l)) {
    return kill_line(c);
  }
  return 0;
}

/* ireturn
 * goto l
 * --------->
 * ireturn
 */
/* Soundness: the goto after the ireturn will never be reached */
int simplify_ireturn_label(CODE **c)
{ int l;
  if(is_ireturn(*c) &&
     is_goto(next(*c),&l)) {
    return replace_modified(c,2,makeCODEireturn(NULL));
  }
  return 0;
}

/* areturn
 * goto l
 * --------->
 * areturn
 */
/* Soundness: the goto after the areturn will never be reached */
int simplify_areturn_label(CODE **c)
{ int l;
  if(is_areturn(*c) &&
     is_goto(next(*c),&l)) {
    return replace_modified(c,2,makeCODEareturn(NULL));
  }
  return 0;
}

/* if_icmpeq true_1
 * iconst_0
 * goto stop_2
 * true_1:
 * iconst_1
 * stop_2:
 * ifeq stop_0
 * ...
 * stop_0:
 * --------->
 * if_icmpne stop_0
 * ...
 * stop_0:
 */
int simplify_if_stmt1(CODE **c)
{ int l1,l2,l3;
  if (is_if(c,&l1) &&
      is_label(nextby(destination(l1),2), &l3) &&
      is_ifeq(nextby(destination(l1),3), &l2)) {
    if (is_if_icmpeq(*c,&l1)) {
      copylabel(l2);
      return replace_modified(c,7,makeCODEif_icmpne(l2,NULL));
    } else if (is_if_icmpgt(*c,&l1)) {
      copylabel(l2);
      return replace_modified(c,7,makeCODEif_icmple(l2,NULL));
    } else if (is_if_icmplt(*c,&l1)) {
      copylabel(l2);
      return replace_modified(c,7,makeCODEif_icmpge(l2,NULL));
    } else if (is_if_icmple(*c,&l1)) {
      copylabel(l2);
      return replace_modified(c,7,makeCODEif_icmpgt(l2,NULL));
    } else if (is_if_icmpge(*c,&l1)) {
      copylabel(l2);
      return replace_modified(c,7,makeCODEif_icmplt(l2,NULL));
    } else if (is_if_icmpne(*c,&l1)) {
      copylabel(l2);
      return replace_modified(c,7,makeCODEif_icmpeq(l2,NULL));
    } else if (is_if_acmpeq(*c,&l1)) {
      copylabel(l2);
      return replace_modified(c,7,makeCODEif_acmpne(l2,NULL));
    } else if (is_if_acmpne(*c,&l1)) {
      copylabel(l2);
      return replace_modified(c,7,makeCODEif_acmpeq(l2,NULL));
    }
  }
  return 0;
}

/* Extension of simplify_if_stmt1 to simplify the following if statement:
 * if (i != 0 && i != 4 && i != 8 ) {}
 */
int simplify_if_stmt2(CODE **c)
{ int l1,l2,l3;
  if (is_if(c,&l1) &&
      is_dup(nextby(destination(l1),3)) &&
      is_ifeq(nextby(destination(l1),4),&l2)) {
    while (is_dup(next(destination(l2))) &&
          is_ifeq(nextby(destination(l2),2),&l2)) {
      /* Iterate to get the last label and update l2 */
    }
    if (is_ifeq(next(destination(l2)),&l3)) {
      if (is_if_icmpeq(*c,&l1)) {
        copylabel(l3);
        return replace_modified(c,9,makeCODEif_icmpne(l3,NULL));
      } else if (is_if_icmpgt(*c,&l1)) {
        copylabel(l3);
        return replace_modified(c,9,makeCODEif_icmple(l3,NULL));
      } else if (is_if_icmplt(*c,&l1)) {
        copylabel(l3);
        return replace_modified(c,9,makeCODEif_icmpge(l3,NULL));
      } else if (is_if_icmple(*c,&l1)) {
        copylabel(l3);
        return replace_modified(c,9,makeCODEif_icmpgt(l3,NULL));
      } else if (is_if_icmpge(*c,&l1)) {
        copylabel(l3);
        return replace_modified(c,9,makeCODEif_icmplt(l3,NULL));
      } else if (is_if_icmpne(*c,&l1)) {
        copylabel(l3);
        return replace_modified(c,9,makeCODEif_icmpeq(l3,NULL));
      } else if (is_if_acmpeq(*c,&l1)) {
        copylabel(l3);
        return replace_modified(c,9,makeCODEif_acmpne(l3,NULL));
      } else if (is_if_acmpne(*c,&l1)) {
        copylabel(l3);
        return replace_modified(c,9,makeCODEif_acmpeq(l3,NULL));
      }
    }
  }
  return 0;
}

/* Extension of simplify_if_stmt1 to simplify the following if statement:
 * if (i == 0 || i == 4 || i == 8 ) {}
 */
int simplify_if_stmt3(CODE **c)
{ int l1,l2,l3;
  if (is_if(c,&l1) &&
      is_dup(nextby(destination(l1),3)) &&
      is_ifne(nextby(destination(l1),4),&l2)) {
    while (is_dup(next(destination(l2))) &&
          is_ifne(nextby(destination(l2),2),&l2)) {
      /* Iterate to get the last label and update l2 */
    }
    if (is_if_icmpeq(*c,&l1)) {
      visit_nodes(*c,9);
      copylabel(l2);
      return replace_modified(c,9,makeCODEif_icmpeq(l2,NULL));
    } else if (is_if_icmpgt(*c,&l1)) {
      visit_nodes(*c,9);
      copylabel(l2);
      return replace_modified(c,9,makeCODEif_icmpgt(l2,NULL));
    } else if (is_if_icmplt(*c,&l1)) {
      visit_nodes(*c,9);
      copylabel(l2);
      return replace_modified(c,9,makeCODEif_icmplt(l2,NULL));
    } else if (is_if_icmple(*c,&l1)) {
      visit_nodes(*c,9);
      copylabel(l2);
      return replace_modified(c,9,makeCODEif_icmple(l2,NULL));
    } else if (is_if_icmpge(*c,&l1)) {
      visit_nodes(*c,9);
      copylabel(l2);
      return replace_modified(c,9,makeCODEif_icmpge(l2,NULL));
    } else if (is_if_icmpne(*c,&l1)) {
      visit_nodes(*c,9);
      copylabel(l2);
      return replace_modified(c,9,makeCODEif_icmpne(l2,NULL));
    }
  } else if (is_if(c,&l1) &&
             is_label(nextby(destination(l1),3),&l2) &&
             is_ifeq(nextby(destination(l1),4),&l3)) {
    if (is_if_icmpeq(*c,&l1)) {
      visit_nodes(*c,8);
      copylabel(l3);
      return replace_modified(c,8,makeCODEif_icmpne(l3,
                                  makeCODElabel(l2,NULL)));
    } else if (is_if_icmpgt(*c,&l1)) {
      visit_nodes(*c,8);
      copylabel(l3);
      return replace_modified(c,8,makeCODEif_icmple(l3,
                                  makeCODElabel(l2,NULL)));
    } else if (is_if_icmplt(*c,&l1)) {
      visit_nodes(*c,8);
      copylabel(l3);
      return replace_modified(c,8,makeCODEif_icmpge(l3,
                                  makeCODElabel(l2,NULL)));
    } else if (is_if_icmple(*c,&l1)) {
      visit_nodes(*c,8);
      copylabel(l3);
      return replace_modified(c,8,makeCODEif_icmpgt(l3,
                                  makeCODElabel(l2,NULL)));
    } else if (is_if_icmpge(*c,&l1)) {
      visit_nodes(*c,8);
      copylabel(l3);
      return replace_modified(c,8,makeCODEif_icmplt(l3,
                                  makeCODElabel(l2,NULL)));
    } else if (is_if_icmpne(*c,&l1)) {
      visit_nodes(*c,8);
      copylabel(l3);
      return replace_modified(c,8,makeCODEif_icmpeq(l3,
                                  makeCODElabel(l2,NULL)));
    }
  }
  return 0;
}

/* Simplify the following if statement since
 * the enclosing bodies assign the same thing.
 *
 * if (resolver == "backtrack" ) {
 *   ss = new BacktrackSolver() ;
 * } else if ( resolver == "bruteforce") {
 *   ss = new BacktrackSolver() ;
 * } else {
 *   ss = new BacktrackSolver() ;
 * }
 * --------->
 * ss = new BacktrackSolver() ;
 */
int simplify_if_stmt4(CODE **c)
{ int a,l1,l2,l3,k1,k2,lines; char *b,*x1,*x2,*y1,*y2;
  if (is_aload(*c,&a) &&
      is_ldc_string(next(*c),&b) &&
      is_if_acmpne(nextby(*c,2),&l1) &&
      is_new(nextby(*c,3),&x1) &&
      is_dup(nextby(*c,4)) &&
      is_invokenonvirtual(nextby(*c,5),&y1) &&
      is_astore(nextby(*c,6),&k1) &&
      is_goto(nextby(*c,7),&l2)) {
    while (is_if_acmpne(nextby(destination(l1),3),&l1)) {
      if (is_new(nextby(destination(l1),4),&x2) &&
          is_dup(nextby(destination(l1),5)) &&
          is_invokenonvirtual(nextby(destination(l1),6),&y2) &&
          is_astore(nextby(destination(l1),7),&k2) &&
          (k1!=k2 || *x1!=*x2 || *y1!=*y2)) {
        return 0;
      }
    }
    if (is_new(next(destination(l1)),&x2) &&
        is_dup(nextby(destination(l1),2)) &&
        is_invokenonvirtual(nextby(destination(l1),3),&y2) &&
        is_astore(nextby(destination(l1),4),&k2)) {
      if (k1!=k2 || *x1!=*x2 || *y1!=*y2) {
        return 0;
      } else {
        lines = 1;
        while (!is_label(nextby(*c,lines),&l3) || l2!=l3) {
          lines++;
        }
        return replace_modified(c,lines,makeCODEnew(x1,
                                        makeCODEdup(
                                        makeCODEinvokenonvirtual(y1,
                                        makeCODEastore(k1,NULL)))));
      }
    }
  }
  return 0;
}

/*
 * Swap simple instructions. Loads and const don't have
 * side effects and are easily swappable at compile time
 * to remove the swap instruction.
 *
 *
 * load or const 1
 * load or const 2
 * swap
 * -------->
 * load or const 2
 * load or const 1
 *
 * Soundness (the first and last lines are the same):
 * [...]
 * [...:x]
 * [...:x:y]
 * [...:y:x]
 * -------->
 * [...]
 * [...:y]
 * [...:y:x]
 */
int precompute_simple_swap(CODE** c)
{ int   itmp;
  char* stmp;
  CODE* c1 = *c;
  CODE* c2 = next(c1);
  CODE* out;

  if (!is_iload(c1, &itmp) &&
      !is_aload(c1, &itmp) &&
      !is_ldc_int(c1, &itmp) &&
      !is_ldc_string(c1, &stmp) &&
      !is_aconst_null(c1) /*&&
      !is_getfield(c1, &stmp)*/)
  {
    return 0;
  }
  if (!is_iload(c2, &itmp) &&
      !is_aload(c2, &itmp) &&
      !is_ldc_int(c2, &itmp) &&
      !is_ldc_string(c2, &stmp) &&
      !is_aconst_null(c2) /*&&
      !is_getfield(c2, &stmp)*/)
  {
    return 0;
  }
  if (!is_swap(nextby(*c,2))) { return 0;}

  switch (c1->kind)
  {
  case getfieldCK:    out = makeCODEgetfield(c1->val.getfieldC, NULL);     break;
  case iloadCK:       out = makeCODEiload(c1->val.iloadC, NULL);           break;
  case aloadCK:       out = makeCODEaload(c1->val.aloadC, NULL);           break;
  case ldc_intCK:     out = makeCODEldc_int(c1->val.ldc_intC, NULL);       break;
  case ldc_stringCK:  out = makeCODEldc_string(c1->val.ldc_stringC, NULL); break;
  case aconst_nullCK: out = makeCODEaconst_null(NULL);                     break;
  default: return 0;
  }

  switch (c2->kind)
  {
  case getfieldCK:    return replace(c, 3, makeCODEgetfield(c2->val.getfieldC, out));
  case iloadCK:       return replace(c, 3, makeCODEiload(c2->val.iloadC, out));
  case aloadCK:       return replace(c, 3, makeCODEaload(c2->val.aloadC, out));
  case ldc_intCK:     return replace(c, 3, makeCODEldc_int(c2->val.ldc_intC, out));
  case ldc_stringCK:  return replace(c, 3, makeCODEldc_string(c2->val.ldc_stringC,
                                                              out));
  case aconst_nullCK: return replace(c, 3, makeCODEaconst_null(out));
  default: return 0;
  }

}

/* iload x
 * aload y
 * swap
 * --------->
 * aload y
 * iload x
 */ 
/* Soundness (same as the last one but with an object):
 * [...]
 * [...:x]
 * [...:x:o]
 * [...:o:x]
 * -------->
 * [...]
 * [...:o]
 * [...:o:x]
 */
int simplify_swap1(CODE **c)
{ int x,y;
  if (is_iload(*c,&x) &&
      is_aload(next(*c),&y) &&
      is_swap(nextby(*c,2))) {
    return replace_modified(c,3,makeCODEaload(y,
                                makeCODEiload(x,NULL)));
  }
  return 0;
}

/* new c
 * dup
 * ldc x
 * invokenonvirtual k
 * aload y
 * swap
 * --------->
 * aload y
 * new c
 * dup
 * ldc x
 * invokenonvirtual k
 *
 *
 * Soundness (the first and last lines are the same, method has same arguments):
 * [...]
 * [...:c]
 * [...:c:c]
 * [...:c:c:x] - method call
 * [...:c]
 * [...:c:y]
 * [...:y:c]
 * -------->
 * [...]
 * [...:y]
 * [...:y:c]
 * [...:y:c:c]
 * [...:y:c:c:x] - method call
 * [...:y:c]
 */
int simplify_swap2(CODE **c)
{ int x,y; char *a,*b;
  if (is_new(*c,&a) &&
      is_dup(next(*c)) &&
      is_ldc_int(nextby(*c,2),&x) &&
      is_invokenonvirtual(nextby(*c,3),&b) &&
      is_aload(nextby(*c,4),&y) &&
      is_swap(nextby(*c,5))) {
    return replace_modified(c,6,makeCODEaload(y,
                                makeCODEnew(a,
                                makeCODEdup(
                                makeCODEldc_int(x,
                                makeCODEinvokenonvirtual(b,NULL))))));
  }
  return 0;
}

/*
 * If a sequence without side effects generates only one item
 * on the stack and it is popped, this sequence hasn't done
 * anything useful and can be removed.
 *
 * We make sure to only test sequences than span a unique
 * basic block.
 *
 * This is sound because we make sure the remove sequence
 * doesn't have side effect such as storing a value or
 * modifying elemments under its starting point.
 *
 *
 * [sequence of instruction without side effect]
 * pop (if stack height of 1)
 * ---------->
 * *nothing*
 *
 * Soundness
 * =========
 * [...:x] <- result of the sequence of instruction
 * [...]   <- popped
 * --------->
 * [...]
 */
int remove_popped_computation(CODE **c)
{ int sh = 0; /* stack height */
  int inc, affected, used;
  CODE* cn = *c;
  int num_instr = 0;
  while (cn != NULL && only_affect_stack(cn))
    {
      stack_effect(cn, &inc, &affected, &used);
      sh += inc;
      /* Means we changed something under our starting point */
      if (affected < -sh) { return 0; }

      num_instr++;
      cn = next(cn);

      /* If next is pop we go a useless computation! */
      if (sh == 1 && is_pop(cn))
        {
          return replace(c, num_instr + 1 ,NULL);
        }
    }
  return 0;
}

/* Soundess:
 * If we find a store we check if it gets overwritten
 * in the rest of the basic block. If it does, storing
 * a value is equivalent to only consuming it.
 *
 * This is sound because we only consider a `straight` sequence
 * of instructions.
 *
 * The generated pop enables other optimisation with
 * `remove_popped_computation`
 *
 * (i|a)store x
 * [sequence with no (a|i)load x, (a|i)store x, no branching in/out]
 * <method end>
 * --------->
 * pop
 * [sequence with no (a|i)load x, (a|i)store x, no branching in/out]
 * <method end>
 */
int unused_store_to_pop(CODE **c)
{
  int x, y;
  CODE* cn = *c;
  if (!is_istore(*c, &x) && !is_astore(*c,&x)) { return 0; }
  do {
    cn = next(cn);
  } while (cn != NULL &&
          !is_if(&cn, &y) &&
          !is_goto(cn, &y) &&
          !is_label(cn, &y) &&
          !is_return(cn) &&
          !is_ireturn(cn) &&
          !is_areturn(cn) &&
          !(is_iload(cn, &y) && x == y) &&
          !(is_aload(cn, &y) && x == y) &&
          !(is_astore(cn, &y) && x == y) && 
          !(is_istore(cn, &y) && x == y));

  if (cn == NULL ||
      is_return(cn) ||
      is_areturn(cn) ||
      is_ireturn(cn) ||
      ((is_astore(cn, &y) || is_istore(cn, &y)) && x == y))
  {
      return replace_modified(c, 1, makeCODEpop(NULL)); /* Stored value unused... */
  }
  return 0;
}

/* isub          isub
 * ifeq L        ifne L
 * --------->    --------->
 * if_icmpeq L   if_icmpne L
 */ 
/* Soundness : if_icmpeq branches when i1 == i2 and isub ifeq branches when i1 == i2 (since ifeq branchges on 0)
 * the same applies for icmpne, but with != instead of ==
 */
int optimize_isub_branching(CODE **c)
{ int L;
  if (!is_isub(*c)) { return 0; }

  if (is_ifne(next(*c), &L))
  {
    return replace_modified(c,2,makeCODEif_icmpne(L, NULL));
  }
  else if (is_ifeq(next(*c), &L))
  {
    return replace_modified(c,2,makeCODEif_icmpeq(L, NULL));
  }
  return 0;
}

/*
 * Soundness: Loading a null and automatically testing for null
 * will be equivalent to a go to....
 *
 * aconst_null
 * ifnull L
 * --------->
 * goto L
 */
int optimize_null_constant_branching(CODE **c)
{ int L;
  if (!is_aconst_null(*c))      { return 0; }
  if (!is_ifnull(next(*c), &L)) { return 0; }
  return replace_modified(c,2,makeCODEgoto(L, NULL));
}

/* ldc_string a      aload i
 * dup               dup
 * ifnull x          ifnull x
 * goto y            goto y
 * label j           label j
 * pop               pop
 * ldc_string b      ldc_string b
 * label k           label k
 * --------->        --------->
 * ldc_string a      aload i
 *                   dup
 *                   ifnonnull y
 *                   pop
 *                   ldc_string b
 *                   label k
 */
int simplify_string_constant(CODE **c)
{ int x,y,i,j,k; char *a,*b;
  if (is_ldc_string(*c,&a) &&
      is_dup(next(*c)) &&
      is_ifnull(nextby(*c,2),&x) &&
      is_goto(nextby(*c,3),&y) &&
      is_label(nextby(*c,4),&j) &&
      is_pop(nextby(*c,5)) &&
      is_ldc_string(nextby(*c,6),&b) &&
      is_label(nextby(*c,7),&k)) {
    return replace_modified(c,8,makeCODEldc_string(a,NULL));
  } else if (is_aload(*c,&i) &&
      is_dup(next(*c)) &&
      is_ifnull(nextby(*c,2),&x) &&
      is_goto(nextby(*c,3),&y) &&
      is_label(nextby(*c,4),&j) &&
      is_pop(nextby(*c,5)) &&
      is_ldc_string(nextby(*c,6),&b) &&
      is_label(nextby(*c,7),&k)) {
    copylabel(y);
    return replace_modified(c,8,makeCODEaload(i,
                                makeCODEdup(
                                makeCODEifnonnull(y,
                                makeCODEpop(
                                makeCODEldc_string(b,
                                makeCODElabel(k,NULL)))))));
  }
  return 0;
}



/**** Unused patterns ****/

/* nop
 * --------->
 * 
 */
/*
int remove_nop(CODE **c)
{ 
  if (is_nop(*c)) {
    return kill_line(c);
  }
  return 0;
}
*/

/*
 * branch_instruction_to L1
 * ...
 * L1:
 * L2:
 * ------>
 * branch_instruction_to L2
 * ...
 * L1:
 * L2:
 */
/*
int point_furthest_label(CODE** c)
{ int L1, L2;
  if(!uses_label(*c, &L1)) { return 0; }
  if(!is_label(next(destination(L1)), &L2)) { return 0; }

  copylabel(L2);

  switch((*c)->kind)
  {
  case gotoCK:      return replace_modified(c, 1, makeCODEgoto(L2, NULL));
  case ifeqCK:      return replace_modified(c, 1, makeCODEifeq(L2, NULL));
  case ifneCK:      return replace_modified(c, 1, makeCODEifne(L2, NULL));
  case ifnullCK:    return replace_modified(c, 1, makeCODEifnull(L2, NULL));
  case ifnonnullCK: return replace_modified(c, 1, makeCODEifnonnull(L2, NULL));
  case if_icmpeqCK: return replace_modified(c, 1, makeCODEif_icmpeq(L2, NULL));
  case if_icmpneCK: return replace_modified(c, 1, makeCODEif_icmpne(L2, NULL));
  case if_icmpgtCK: return replace_modified(c, 1, makeCODEif_icmpgt(L2, NULL));
  case if_icmpltCK: return replace_modified(c, 1, makeCODEif_icmplt(L2, NULL));
  case if_icmpleCK: return replace_modified(c, 1, makeCODEif_icmple(L2, NULL));
  case if_icmpgeCK: return replace_modified(c, 1, makeCODEif_icmpge(L2, NULL));
  case if_acmpeqCK: return replace_modified(c, 1, makeCODEif_icmpeq(L2, NULL));
  case if_acmpneCK: return replace_modified(c, 1, makeCODEif_icmpne(L2, NULL));
  default:          return 0;
  }
}
*/



/****** Old style - still works, but better to use new style.
#define OPTS 4

OPTI optimization[OPTS] = {simplify_multiplication_right,
                           simplify_astore,
                           positive_increment,
                           simplify_goto_goto};
********/

/* new style for giving patterns */

int init_patterns()
{ 
/*
  ADD_PATTERN(remove_nop);

  Incompatible with simplify ifs_stmt#
  ADD_PATTERN(point_furthest_label);

  remove_popped_computation handles this
  ADD_PATTERN(remove_dup_pop);

  Done by precompute simple swap
  ADD_PATTERN(simplify_const_load_swap);
  ADD_PATTERN(simplify_swap1);
*/

  ADD_PATTERN(simplify_multiplication_right);
  ADD_PATTERN(simplify_astore);
  ADD_PATTERN(positive_increment);
  ADD_PATTERN(simplify_goto_goto);
  ADD_PATTERN(simplify_multiplication_left);
  ADD_PATTERN(simplify_istore);
  ADD_PATTERN(simplify_aload);
  ADD_PATTERN(optimize_astore);
  ADD_PATTERN(optimize_istore);
  ADD_PATTERN(simplify_iload);
  ADD_PATTERN(simplify_aload_getfield_aload_swap);
  ADD_PATTERN(simplify_aload_swap_putfield);
  ADD_PATTERN(simplify_constant_op);
  ADD_PATTERN(remove_superfluous_return);
  ADD_PATTERN(simplify_trivial_op);
  ADD_PATTERN(remove_2_swap);
  ADD_PATTERN(remove_aload_astore);
  ADD_PATTERN(remove_iload_istore);
  ADD_PATTERN(remove_deadlabel);
  ADD_PATTERN(simplify_ireturn_label);
  ADD_PATTERN(simplify_areturn_label);
  ADD_PATTERN(simplify_if_stmt1);
  ADD_PATTERN(simplify_if_stmt2);
  ADD_PATTERN(simplify_if_stmt3);
  ADD_PATTERN(simplify_if_stmt4);
  ADD_PATTERN(precompute_simple_swap);
  ADD_PATTERN(simplify_swap2);

  ADD_PATTERN(unused_store_to_pop);
  ADD_PATTERN(remove_popped_computation);
  ADD_PATTERN(optimize_isub_branching);
  ADD_PATTERN(optimize_null_constant_branching);
  ADD_PATTERN(simplify_string_constant);

  return 1;
}
