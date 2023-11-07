#include <iostream>
#include "globals.hpp"

int main() {
  load_mem("code", 0);
  load_mem("data", 0x2000);
  
  init();

  run();
  dump_mem(0, 0x810, 'h');
  dump_breg('h');

  return 0;
};

