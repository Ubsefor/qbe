//
//  4licm.c
//  QBE
//
//  Created by Alexander Makhov on 31.03.2022.
//

#include <qbe/all.h>
#include <stdio.h>
#include <stdbool.h>

// opcode info
typedef struct {
  Blk *block;
  int bl_idx;
  int opcode_idx;
} invar_opcode_t;

// invar opcodes array
typedef struct {
  invar_opcode_t *values;
  int size;
  int length;
} info_arr_t;

// blocks array
typedef struct {
  Blk **blocks;
  uint max_id;
  int size;
  int length;
} blk_arr_t;

// Array helper funcs
// array init
info_arr_t init_array(void) {
  info_arr_t array;
  array.size = 0;
  array.length = 1;
  array.values = (invar_opcode_t *) malloc(sizeof(invar_opcode_t));
  return array;
}

// check if opcode is among invars
bool is_invar_opc(info_arr_t array, Ins *ins) {
  for (int i = 0; i < array.size; i++) {
    invar_opcode_t info = array.values[i];

    if (info.block->ins + info.opcode_idx == ins)
      return true;
  }

  return false;
}

// add to array of invar blocks
void join_info_arr(info_arr_t *array, Blk *block, int block_idx, int opcode_idx) {
  array->values[array->size].block = block;
  array->values[array->size].bl_idx = block_idx;
  array->values[array->size].opcode_idx = opcode_idx;
  array->size++;

  if (array->size >= array->length) {
    array->length *= 2;
    array->values = (invar_opcode_t *) realloc(array->values, array->length * sizeof(invar_opcode_t));
  }
}


// block array helpers

// block array init
blk_arr_t init_blk_arr(void) {
  blk_arr_t array;
  array.size = 0;
  array.length = 1;
  array.blocks = (Blk **) malloc(sizeof(Blk *));
  return array;
}

// check if block present in array
bool blk_present(blk_arr_t array, Blk *blk) {
  for (int i = 0; i < array.size; i++)
    if (array.blocks[i] == blk)
      return true;

  return false;
}

// add block to block array
bool join_blk_arr(blk_arr_t *array, Blk *blk) {
  if (blk_present(*array, blk))
    return false;

  array->blocks[array->size] = blk;

  if (blk->id > array->max_id)
    array->max_id = blk->id;

  array->size++;

  if (array->size >= array->length) {
    array->length *= 2;
    array->blocks = (Blk **) realloc(array->blocks, array->length * sizeof(Blk *));
  }

  return true;
}

// check if arc exists b/w blocks

bool is_fwd_edge(Blk *a, Blk *b) {
  assert(a->s1 == b || a->s2 == b);

  if (a->id && b->id)
    return a->id < b->id;

  return a->id == 0;
}

bool is_back_edge(Blk *a, Blk *b) {
  assert(a->s1 == b || a->s2 == b);

  if (a->id && b->id)
    return a->id >= b->id;

  return false;
}

// get invars

// check for multiple definitions
int check_mult_defs(Fn *fn, blk_arr_t blocks, Ref arg) {
  for (int k = 0; k < blocks.size; k++) {
    for (Phi *phi = blocks.blocks[k]->phi; phi; phi = phi->link) {

      if (phi->to.val != arg.val)
        continue;

      for (int i = 0; i < phi->narg; i++)
        if (blk_present(blocks, phi->blk[i]))
          return 0;

      return 1;
    }
  }

  return -1;
}

// check if reaching definition is in loop
bool is_def_in_loop(blk_arr_t blocks, Ref arg) {
  for (int i = 0; i < blocks.size; i++)
    for (int j = 0; j < blocks.blocks[i]->nins; j++)
      if (blocks.blocks[i]->ins[j].to.val == arg.val)
        return true;

  return false; // ¿no definitions?
}

// rd is marked as invar
bool is_def_invar(info_arr_t invar_opc_arr, Ref arg) {
  for (int i = 0; i < invar_opc_arr.size; i++) {
    invar_opcode_t info = invar_opc_arr.values[i];

    if (arg.val == info.block->ins[info.opcode_idx].to.val)
      return true;
  }

  return false;
}

// check if arg is invar
bool is_arg_invar(Fn *fn, blk_arr_t blocks, info_arr_t invar_opc_arr, Ref arg, Blk *blk) {
  // константа инвариантна
  if (arg.type == RCon)
    return true;

  // not invar if theres multiple rd and one is inside loop
  int phi = check_mult_defs(fn, blocks, arg);

  if (phi == 1) // several rd
    return true;

  if (phi == 0) // several rd, 1 in a loop
    return false;
  // exists 1 rd

  // var is invar, if theres no its def in a loop
  if (!is_def_in_loop(blocks, arg))
    return true;

  // var is defined by opcode in a loop

  // invar, if rd is already marked as invar
  return is_def_invar(invar_opc_arr, arg);
}

// check that opcode is a loop invar
bool is_new_invar(Fn *fn, blk_arr_t blocks, info_arr_t invar_opc_arr, Ins *ins, Blk *blk) {
  // not a new invar if already marked as invar
  if (is_invar_opc(invar_opc_arr, ins))
    return false;

  // opcode is invar if both args are invars
  if (!is_arg_invar(fn, blocks, invar_opc_arr, ins->arg[0], blk))
    return false;

  if (!is_arg_invar(fn, blocks, invar_opc_arr, ins->arg[1], blk))
    return false;

  return true;
}

// collect all invars inside a loop
info_arr_t get_invar_opcodes(Fn *fn, blk_arr_t blocks) {
  info_arr_t invar_opc_arr = init_array();
  int was_added = 1;

  while (was_added) {
    was_added = 0;

    for (int i = 0; i < blocks.size; i++) {
      for (int j = 0; j < blocks.blocks[i]->nins; j++) {
        if (is_new_invar(fn, blocks, invar_opc_arr, blocks.blocks[i]->ins + j, blocks.blocks[i])) {
          join_info_arr(&invar_opc_arr, blocks.blocks[i], i, j);
          was_added = 1;
        }
      }
    }
  }

  return invar_opc_arr;
}

// Create preheader

// update pred of preheader and header
void update_pred(Blk **preds, int npreds, Blk *prehead, Blk *first) {
  Blk **prehead_preds = (Blk **) alloc(npreds * sizeof(Blk *));
  Blk **first_preds = (Blk **) alloc(npreds * sizeof(Blk *));

  int index1 = 0;
  int index2 = 0;

  // arc from preheader into a header
  first_preds[index1++] = prehead;

  for (int i = 0; i < npreds; i++) {
    // leave reverse directed arcs to a header
    if (is_back_edge(preds[i], first)) {
      first_preds[index1++] = preds[i];
    }
    else {
      // transfer forward arcs from header to preheader
      if (preds[i]->s1 == first)
        preds[i]->s1 = prehead;

      if (preds[i]->s2 == first)
        preds[i]->s2 = prehead;

      prehead_preds[index2++] = preds[i];
    }
  }

  first->pred = first_preds;
  first->npred = index1;
  prehead->pred = prehead_preds;
  prehead->npred = index2;
}

// update function blocks
void update_fn(Fn *fn, Blk *prehead, Blk *first) {
  fn->nblk++;
  prehead->link = first;

  if (first == fn->start) {
    fn->start = prehead;
    return;
  }

  Blk *blk = fn->start;

  while (blk->link != first)
    blk = blk->link;

  blk->link = prehead;
}

// make preheader
Blk* make_prehead(Fn *fn, Blk *first) {
  Blk *prehead = (Blk *) alloc(sizeof(Blk));

  strcpy(prehead->name, "prehead@");
  strcat(prehead->name, first->name);

  // if preheader was created earlier – return it
  for (int i = 0; i < first->npred; i++)
    if (!strcmp(first->pred[i]->name, prehead->name))
      return first->pred[i];

  // create connections b/w blocks
  prehead->s1 = first;
  prehead->s2 = NULL;
  prehead->jmp.type = Jjmp;

  update_pred(first->pred, first->npred, prehead, first);
  update_fn(fn, prehead, first);

  // while preheader is empty
  prehead->ins = NULL;
  prehead->nins = 0;

  return prehead;
}

// moving of invar opcodes

// if def is in block that is dom of all loop exits – it can be moved
bool dom_exists(blk_arr_t blocks, Blk *block) {
  for (int i = 0; i < blocks.size; i++) {
    // skip blocks w/o loop exits
    if (!blocks.blocks[i]->s2 || blk_present(blocks, blocks.blocks[i]->s2))
      continue;

    if (!dom(block, blocks.blocks[i]))
      return false;
  }

  return true;
}

// check if var is in use after loop
bool is_used_after_loop(Fn *fn, uint max_id, int val) {
  for (int i = 0; i < fn->tmp[val].nuse; i++)
    if (fn->tmp[val].use[i].bid > max_id)
      return true;

  return false;
}

// check if opcode can be moved
bool can_move(Fn *fn, invar_opcode_t info, blk_arr_t blocks) {
  Ins *ins = info.block->ins + info.opcode_idx;

  // def cant be moved if loop has another def of this var,
  // but ssa makes these defs independent, so theres no need to check it here

  // def can be moved if its in a block that doms all loop exits
  if (dom_exists(blocks, info.block))
    return true;

  // ... or if it is killed after loop (theres no futher uses)
  if (!is_used_after_loop(fn, blocks.max_id, ins->to.val))
    return true;

  return false;
}

// move invar opcodes
void move_opcodes(Fn *fn, blk_arr_t blocks, Blk *prehead, Blk *last, info_arr_t invar_opc_arr) {
  Ins *prehead_ins = (Ins *) alloc((prehead->nins + invar_opc_arr.size) * sizeof(Ins));
  memcpy(prehead_ins, prehead->ins, prehead->nins * sizeof(Ins));
  prehead->ins = prehead_ins;

  Ins* blocks_ins[blocks.size];
  int blocks_ins_sizes[blocks.size];

  // copy info of blocks into new arrays,
  // which we are gonna modify
  // and fing max id b/w new loop blocks
  for (int i = 0; i < blocks.size; i++) {
    blocks_ins_sizes[i] = blocks.blocks[i]->nins;
    blocks_ins[i] = (Ins *) alloc(blocks.blocks[i]->nins * sizeof(Ins));

    memcpy(blocks_ins[i], blocks.blocks[i]->ins, blocks.blocks[i]->nins * sizeof(Ins));
  }

  // try to move invar opcodes
  for (int i = 0; i < invar_opc_arr.size; i++) {
    invar_opcode_t info = invar_opc_arr.values[i];

    // some cant be moved
    if (!can_move(fn, info, blocks))
      continue;

    // add opcode to a preheader
    prehead->ins[prehead->nins++] = info.block->ins[info.opcode_idx];

    // delete from inside a loop
    int shift = info.block->nins - blocks_ins_sizes[info.bl_idx];
    int end_index = --blocks_ins_sizes[info.bl_idx];

    for (int j = info.opcode_idx - shift; j < end_index; j++)
      blocks_ins[info.bl_idx][j] = blocks_ins[info.bl_idx][j + 1];
  }

  // update arrays of opcodes for blocks
  for (int i = 0; i < blocks.size; i++) {
    blocks.blocks[i]->nins = blocks_ins_sizes[i];
    blocks.blocks[i]->ins = blocks_ins[i];
  }
}

// parse loops
// get blocks, that are final exits from a loop
blk_arr_t get_lasts(Fn *fn) {
  blk_arr_t lasts = init_blk_arr();

  for (Blk *blk = fn->start; blk; blk = blk->link)
    for (int i = 0; i < blk->npred; i++)
      if (is_back_edge(blk->pred[i], blk))
        join_blk_arr(&lasts, blk->pred[i]);

  return lasts;
}

// get loop blocks
void get_loop_blocks(Blk *last, Blk *curr, blk_arr_t *blocks) {
  if (!join_blk_arr(blocks, curr))
    return;

  if ((last->s1 == curr || last->s2 == curr) && is_back_edge(last, curr))
    return;

  for (int i = 0; i < curr->npred; i++)
    if (curr->pred[i]->id != curr->id)
      get_loop_blocks(last, curr->pred[i], blocks);
}

// loop parsing
void parse_loop(Fn *fn, Blk *first, Blk *last, blk_arr_t blocks) {
  // find invar opcodes
  info_arr_t invar_opc_arr = get_invar_opcodes(fn, blocks);

  // вставляем и заполняем предзаголовок только если есть инварианты
  // insert and fill a preheader if there are invars
  if (invar_opc_arr.size) {
    Blk *prehead = make_prehead(fn, first);
    move_opcodes(fn, blocks, prehead, last, invar_opc_arr);
  }

  free(invar_opc_arr.values);
  free(blocks.blocks);
}

// find loop which has a reverse arc using block and parse it
void parse_as_footer(Fn *fn, Blk *blk) {
  // find loop blocks
  blk_arr_t blocks = init_blk_arr();
  get_loop_blocks(blk, blk, &blocks);

  if (blk_present(blocks, fn->start) && blk->s1 != fn->start && blk->s2 != fn->start) {
    free(blocks.blocks);
    return;
  }

  // substitute header and parse loop
  // header -- block, to which goes current arc
  if (is_back_edge(blk, blk->s1)) {
    parse_loop(fn, blk->s1, blk, blocks);
    return;
  }

  assert(is_back_edge(blk, blk->s2));
  parse_loop(fn, blk->s2, blk, blocks);
}

// delete empty preheaders

void remove_prehead(Fn *fn, Blk *prehead) {
  Blk *head = prehead->s1;

  for (int i = 0; i < prehead->npred; i++) {
    if (prehead->pred[i]->s1 == prehead)
      prehead->pred[i]->s1 = head;

    if (prehead->pred[i]->s2 == prehead)
      prehead->pred[i]->s2 = head;

    head->pred[i ? head->npred++ : 0] = prehead->pred[i];
  }

  fn->nblk--;

  if (prehead == fn->start) {
    fn->start = prehead->link;
    return;
  }

  Blk *blk = fn->start;

  while (blk->link != prehead)
    blk = blk->link;

  blk->link = prehead->link;
}

// check if prehead is empty and remove
void remove_empty_preheads(Fn *fn) {
  for (Blk *blk = fn->start; blk; blk = blk->link)
    if (!strncmp("prehead@", blk->name, 8) && blk->nins == 0)
      remove_prehead(fn, blk);
}

// template

static void readfn (Fn *fn) {
  fillrpo(fn); // Traverses the CFG in reverse post-order, filling blk->id.
  fillpreds(fn);
  filluse(fn);
  ssa(fn);
  filluse(fn);

  // code
  blk_arr_t lasts = get_lasts(fn);

  for (int i = 0; i < lasts.size; i++)
    parse_as_footer(fn, lasts.blocks[i]);

  free(lasts.blocks);

  remove_empty_preheads(fn);

  // code end

  printfn(fn, stdout);
}

static void readdat (Dat *dat) {
  (void) dat;
}

int main () {
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}
