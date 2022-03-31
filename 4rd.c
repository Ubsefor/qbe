//
//  4rd.c
//  QBE
//
//  Created by Alexander Makhov on 31.03.2022.
//

#include <qbe/all.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
  char shortname[50];
  char fullname[70];
} tmps;

typedef struct {
  // contains the short name for gen, kill and rd
  tmps gen[200];
  int genlen;
  tmps kill[200];
  int killlen;
  tmps in[200];
  int inlen;
} data;

typedef struct {
  Blk* key;
  data* value;
} mapper;

data* info;
mapper* map;
int map_length = 0;

data* get_info(Blk *block) {
  for (int i = 0; i < map_length; i++) {
    if (map[i].key == block) {
      return map[i].value;
    }
  }
  return NULL;
}

static int find_short_name(char* name, tmps* array, int length) {
  for (int index = 0; index<length; index++) {
    if (strcmp(array[index].shortname, name) == 0) {
      return index;
    }
  }
  return -1;
}

static int find_full_name(char* name, tmps* array, int length) {
  for (int index = 0; index<length; index++) {
    if (strcmp(array[index].fullname, name) == 0) {
      return index;
    }
  }
  return -1;
}

static tmps format_name(Blk* blk, Tmp t) {
  tmps variable;
  // for gen/kill analysis
  sprintf(variable.shortname, "%s", t.name);
  // for rd analysis
  sprintf(variable.fullname, "@%s%%%s", blk->name, t.name);
  return variable;
}

static bool parse_blocks(Blk* blk) {
  bool change = false;
  // arrays contain to.val
  data* blk_info = get_info(blk);
  tmps child_in[blk_info->genlen + blk_info->inlen];
  int ch_in_len = 0;
  for (int i=0; i < blk_info->inlen; i++) {
    int index = find_full_name(blk_info->in[i].fullname, blk_info->kill, blk_info->killlen);
    if (index < 0) {
      // if not found, then this IN var is transitive; send it to the next blk
      child_in[ch_in_len++] = blk_info->in[i];
    }
  }

  for (int i=0; i < blk_info->genlen; i++) {
    child_in[ch_in_len++] = blk_info->gen[i];
  }

  if (blk->s1) {
    Blk* lchild = blk->s1;
    data* linfo = get_info(lchild);
    // while we have out genvars, add them to next block info
    for (int i=0; i < ch_in_len; i++) {
      int index = find_full_name(child_in[i].fullname, linfo->in, linfo->inlen);
      if (index < 0) {
        // something new
        linfo->in[linfo->inlen++] = child_in[i];
        change = true;
      }
    }
  }

  if (blk->s2) {
    Blk* rchild = blk->s2;
    data* rinfo = get_info(rchild);
    // while we have out genvars, add them to next block info
    for (int i=0; i < ch_in_len; i++) {
      int index = find_full_name(child_in[i].fullname, rinfo->in, rinfo->inlen);
      if (index < 0) {
        // something new
        rinfo->in[rinfo->inlen++] = child_in[i];
        change = true;
      }
    }
  }
  return change;
}

static void gen_kill(Fn* fn) {
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    // arrays contain to.val
    data* info = get_info(blk);

    for (Ins* i=blk->ins; i-blk->ins < blk->nins; i++) {
      // if we are assigning to a temporary variable
      if (Tmp0 > i->to.val) {
        continue;
      }
      char* s1 = fn->tmp[i->to.val].name;
      int index = find_short_name(s1, info->gen, info->genlen);
      if (index >= 0) {
        // not unique
        continue;
      }
      // -> gen var in this block
      info->gen[info->genlen++] = format_name(blk, fn->tmp[i->to.val]);
    }

    // gen_count--
    int k = info->genlen;
    while (k-- > 0) {
      // search where we might kill this var
      // look through all other blocks and instructions
      tmps vnew = info->gen[k];
      for (Blk *blk_ = fn->start; blk_; blk_ = blk_->link) {
        if (blk_ == blk) {
          // same block
          continue;
        }
        for (Ins* in_=blk_->ins; in_-blk_->ins < blk_->nins; in_++) {
          if (Tmp0 > in_->to.val) {
            // not a var
            continue;
          }
          tmps vnew_ = format_name(blk_, fn->tmp[in_->to.val]);
          if (strcmp(vnew.shortname, vnew_.shortname) != 0) {
            // the other def is for another variable
            continue;
          }
          int idx = find_full_name(vnew_.fullname, info->kill, info->killlen);
          if (idx >= 0) {
            // already killed this var in that block
            continue;
          }
          info->kill[info->killlen++] = format_name(blk_, fn->tmp[in_->to.val]);
          // then we kill this instruction, that is assigned to the same var
        }
      }
    }
  }

}

static void readfn (Fn *fn) {
  map = (mapper*) malloc(fn->nblk * sizeof(mapper));
  map_length = fn->nblk;
  info = (data*) malloc(fn->nblk * sizeof(data));
  int mm_i = 0;
  for (Blk* blk = fn->start; blk; blk = blk->link) {
    // set len of info to zero
    info[mm_i].inlen = 0;
    info[mm_i].genlen = 0;
    info[mm_i].killlen = 0;
    // map info to block
    map[mm_i].key = blk;
    map[mm_i].value = &info[mm_i];
    // for every block
    mm_i++;
  }

  gen_kill(fn);

  bool change = true;
  while (change) {
    change = false;
    for (Blk* blk = fn->start; blk; blk = blk->link) {
      if( parse_blocks(blk) ) {
        change = true;
      }
    }
  }

  for (Blk* blk = fn->start; blk; blk = blk->link) {
    printf("@%s", blk->name);
    printf("\n\trd_in =");
    // modified initial list, it is used for info of next block
    data* meta = get_info(blk);
    for (int i=0; i < meta->inlen; i++) {
      printf(" %s", meta->in[i].fullname);
    }
    printf("\n");
  }
  printf("\n");

  free(map);
  free(info);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}
