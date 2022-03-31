//
//  4rd.cpp
//  4rd
//
//  Created by Alexander Makhov on 31.03.2022.
//

#define export exports
extern "C" {
#include "qbe/all.h"
}
#undef export
#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

using namespace std;

Target T;

vector<Blk *> blocks;
vector<string> blockNames;
vector< vector<string> > gens;
vector< vector<string> > kills;
vector< vector<string> > ins;
vector< vector<string> > outs;

static void readfn(Fn *fn)
{
  for (Blk *blk = fn->start; blk; blk = blk->link) {
    vector<string> gen{};

    blocks.push_back(blk);
    blockNames.push_back(blk->name);
    for (auto i = blk->ins; i < &blk->ins[blk->nins]; ++i) {
      if (fn->tmp[i->to.val].name[0] != '\0' &&
          std::find(gen.begin(), gen.end(), fn->tmp[i->to.val].name) == gen.end()) {
        gen.push_back(fn->tmp[i->to.val].name);
      }
    }

    gens.push_back(gen);
  }

  for (auto j = 0; j < gens.size(); j++) {
    vector<string> kill{};
    for (int k = 0; k < gens[j].size(); k++) {
      for (auto i = 0; i < gens.size(); i++) {
        if (i == j) {
          continue;
        }
        if (std::find(gens[i].begin(), gens[i].end(), gens[j][k]) != gens[i].end()) {
          string str = "@" + string(blocks[i]->name) + "%" + string(gens[j][k]);
          if (std::find(kill.begin(), kill.end(), str) == kill.end()) {
            kill.push_back(str.c_str());
          }
          break;
        }
      }
    }
    kills.push_back(kill);
  }

  for (auto i = 0; i < blocks.size(); i++) {
    ins.push_back(std::vector<string>());
    outs.push_back(std::vector<string>());
    for (auto& gg : gens[i]) {
      gg = "@" + string(blocks[i]->name) + "%" + string(gg);
    }
  }

  // cout << "GENS:" << endl;
  // for (auto ii : gens) {
  //     cout << endl;
  //     for (auto iii : ii) {
  //         cout << iii << endl;
  //     }
  // }

  // cout << "KILLS:" << endl;
  // for (auto ii : kills) {
  //     cout << endl;
  //     for (auto iii : ii) {
  //         cout << iii << endl;
  //     }
  // }
  // cout << "-------------------" << endl;
  // for (auto i = 0; i < blocks.size(); i++) {
  //     cout << "BLOCK:" << blocks[i] << endl;
  // }

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto i = 0; i < blocks.size(); i++) {
      ins[i] = {};
      if (blocks[i]->npred) {
        auto preds = blocks[i]->pred;
        for (auto l = 0; l < blocks[i]->npred; l++) {
          auto name = string(preds[l]->name);
          auto it = std::find(blockNames.begin(), blockNames.end(), name);
          int index = std::distance(blockNames.begin(), it);
          for (auto& oo : outs[index]) {
            if (std::find(ins[i].begin(), ins[i].end(), oo) == ins[i].end()) {
              ins[i].push_back(oo);
            }
          }
        }
      }

      // cout << "INS:" << i << endl;
      // for (auto iii : ins[i])
      // {
      //         cout << iii << endl;
      // }

      auto newOut = std::vector<string>(gens[i].begin(), gens[i].end());
      vector<string> diff(ins[i].begin(), ins[i].end());

      for (auto kk : kills[i]) {
        auto it = std::find(diff.begin(), diff.end(), kk);
        if (it != diff.end()) {
          diff.erase(it);
        }
      }
      for (auto dd : diff) {
        if (std::find(newOut.begin(), newOut.end(), dd) == newOut.end()) {
          newOut.push_back(dd);
        }
      }

      std::set<string> s1(newOut.begin(), newOut.end());
      std::set<string> s2(outs[i].begin(), outs[i].end());
      if (!std::equal(s1.begin(), s1.end(), s2.begin())) {
        // cout << "NEWOUTS:" << i<< endl;
        // for (auto oo : newOut)
        // {
        //     cout << oo << endl;
        // }
        // cout << "OUTS:"<< i << endl;
        // for (auto oo : outs[i])
        // {
        //     cout << oo << endl;
        // }
        outs[i] = vector<string>(newOut.begin(), newOut.end());
        changed = true;
      }
      // cout << endl;
    }
  }
  // cout << "=============================="  << endl;
  // }

  for (auto i = 0; i < blocks.size(); i++) {
    printf("@%s", blocks[i]->name);

    printf("\n\trd_in = ");
    for (auto ii : ins[i]) {
      printf("%s ", ii.c_str());
    }

    printf("\n\n");
  }
}

static void readdat(Dat *dat)
{
  (void)dat;
}

int main()
{
  parse(stdin, "<stdin>", readdat, readfn);
  freeall();
}
