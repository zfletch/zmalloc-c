#include <stdio.h>
#include "../src/zmalloc.h"

// gcc -Wall -Wextra hello.c ../src/zmalloc.c
// prints "hello world" to stdout
int main()
{
  char *world;
  
  world = zmalloc(6 * sizeof(char));
  world[0] = 'w'; world[1] = 'o'; world[2] = 'r';
  world[3] = 'l'; world[4] = 'd'; world[5] = '\0';
  
  printf("hello %s\n", world);
  
  zfree(world);
  
  return 0;
}
