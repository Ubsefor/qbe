//
//  4licm.c
//  QBE
//
//  Created by Alexander Makhov on 31.03.2022.
//

#include <qbe/all.h>
#include <stdio.h>

static void readfn (Fn *fn) {
  fillrpo(fn); // Traverses the CFG in reverse post-order, filling blk->id.
  fillpreds(fn);
  filluse(fn);
  ssa(fn);

  // some shit here

  printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}
