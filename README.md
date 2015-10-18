# Memory Allocator

This is a memory allocator in C. It uses the [buddy system](https://en.wikipedia.org/wiki/Buddy_memory_allocation)
to avoid fragmentation. The public interface includes the standard `malloc`, `calloc`, `realloc`, and `free`.

## Example
```c
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
```

## Public Interface
```c
// given an unsigned integer, size
// returns a pointer to a place in memory with at least size bytes
// will be 8-byte (double-word) aligned
// will return NULL if memory cannot be allocated or 0 is passed in
void* zmalloc(size_t size);

// deallocates memory allocated by zmalloc
void zfree(void* ptr);

// changes the size of the block that ptr points to
// possibly moving the memory to somewhere else
void* zrealloc(void* ptr, size_t size);

// allocates num * size bytes and initializes them to 0
void* zcalloc(size_t num, size_t size);
```
