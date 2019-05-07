#include "mem.h"
#include <stdio.h>
#include <string.h>

int main (int argc, char **argv) {
  // Get a big chunk of memory from the OS for the memory
  // allocator to manage.
  void *memory = initMem (4096);
  printf ("initMem returned %p\n\n", memory);

  // We should see one big chunk.
  dumpMem();

  // Allocate 4 bytes of memory and store "abc" in that address
  void *ptr = allocMem (4);
  printf ("Allocated 4 bytes at %p\n", ptr);
  strcpy(ptr, "abc");

  // Still one chunk of free memory but it is smaller
  dumpMem();

  // Free the block just allocated.
  printf ("Freeing the allocated chunk.\n");
  if (freeMem(ptr) == -1) {
    printf ("ERROR:  freeMem failed!\n");
  }


  // Should have 2 chunks of free memory:  one at the block from
  // the previous list and one at the start of the memory.
  dumpMem();

  // Try to free the pointer again.  This should fail.
  freeMem(ptr);
  if (freeMem(ptr) != -1) {
    printf ("ERROR:  freeMem of same pointer should have failed!\n");
  }

  // Allocate 2 chunks of memory
  printf ("Allocating 2 chunks of memory.\n");
  char *ptr2 = allocMem(4);
  strcpy (ptr2, "mhc");
  char *ptr3 = allocMem(4);
  strcpy (ptr3, "bos");

  // Should see 1 chunk
  dumpMem();

  // Freeing the first chunk and asking for memory that should come from the
  // second free chunk.
  printf ("Freeing first chunk and allocating a 3rd bigger chunk.\n");
  if (freeMem(ptr2) == -1) {
    printf ("ERROR:  freeMem failed!\n");
  }
  char *ptr4 = allocMem(11);
  strcpy (ptr4, "0123456789");

  // Should see 2 chunks
  dumpMem();

  // Allocate a chunk that should fit in the first free block
  printf ("Reallocating from first chunk.\n");
  char *ptr5 = allocMem(4);
  strcpy (ptr5, "csc");

  // Should see 1 chunk
  dumpMem();

  // Verify that memory that was set and not freed has not changed.
  if (strcmp (ptr3, "bos")) {
    printf ("ERROR: ptr3 changed to %s\n", ptr3);
  }
  
  if (strcmp (ptr4, "0123456789")) {
    printf ("ERROR: ptr4 changed to %s\n", ptr4);
  }
  
  if (strcmp (ptr5, "csc")) {
    printf ("ERROR: ptr5 changed to %s\n", ptr5);
  }
  
  // Allocate 4000 bytes
  printf ("Allocate a big blocks\n");
  void *ptr6 = allocMem (4000);
  printf ("Allocated 4000 bytes at %p\n", ptr6);
  strcpy (ptr6, "This is a big block.\n");

  // Still one chunk of free memory but much smaller
  dumpMem();

  // This allocation should fail
  void *ptr7 = allocMem (1000);
  printf ("Tried to allocate 1000 bytes.  allocMem returned %p\n", ptr7);
  dumpMem();

  printf ("Freeing a random address; it should fail\n");
  if (freeMem((void *) ptr3 + 4) != -1) {
    printf ("ERROR:  freeMem should have failed!\n");
  }
  
  dumpMem();
}
