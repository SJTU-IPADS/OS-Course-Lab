#include <stdio.h>
#include "phases.h"
#include "utils.h"

int main() {
  char* input;
  printf("Type in your defuse password!\n");

  input = read_line();
  phase_0(input);
  phase_defused();

  input = read_line();
  phase_1(input);
  phase_defused();

  input = read_line();
  phase_2(input);
  phase_defused();

  input = read_line();
  phase_3(input);
  phase_defused();

  input = read_line();
  phase_4(input);
  phase_defused();

  input = read_line();
  phase_5(input);
  phase_defused();

  printf("Congrats! You have defused all phases!\n");
  return 0;
}