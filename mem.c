#include <sys/types.h> 
#include <sys/mman.h> 
#include <err.h> 
#include <fcntl.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdbool.h> 

/* PLEASE READ: Because my header size is larger than the one assumed in memtest, 
one of the test cases does not match up accordingly (in memtest, after allocated 4000
bytes, the test says there is still one chunk of free memory left, however
this is not the case for my implementation since 16 bytes > 12 bytes for header size */

#define MEM_SIZE 4096 //for testing

#define MAGIC_NUMBER 123456789L 

//Struct for a free block
typedef struct FreeNode { 
   int size; 
   struct FreeNode *next; 
} FreeNode; 

//Struct for an allocated block 
typedef struct AllocNode { 
   int size; 
   long magicNumber; 
} AllocNode; 

FreeNode *gFreeNodePtr = NULL; //global pointer to a free node 

#define HEADER_SIZE sizeof(FreeNode) //size of header is 16 bytes total 

/* Calls mmap to request sizeOfRegion bytes of memory to manage */ 
void *initMem(int sizeOfRegion) { 
  void *ptr; 

  // open the /dev/zero device 
  int fd = open("/dev/zero", O_RDWR); 

  // sizeOfRegion (in bytes) needs to be evenly divisible by the mem size 
  if (sizeOfRegion % MEM_SIZE == 0) { 
    ptr = mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0); 
    if (ptr == MAP_FAILED) { 
      perror("mmap: failed to allocate"); 
      exit(1); 
    } 
  } else { 
    perror("mmap: invalid size requested"); 
    exit(2); 
  } 
  // close the device (mapping should be unaffected) 
  close(fd); 

  // creating a new free node header at theh start of the entire block
  // since entire block is free at initialization 
  // assigning ptr from mmap to our global free node ptr
  gFreeNodePtr = (FreeNode *)ptr; 
  gFreeNodePtr->size = sizeOfRegion - HEADER_SIZE; 
  gFreeNodePtr->next = NULL; 

  return ptr;
} 

/* Takes size (bytes) to be allocated and returns a pointer to start. 
If not enough contiguous free space to hold requested size, returns NULL */ 
void *allocMem(int size) { 
  FreeNode *prevFreeNodePtr = NULL; 
  FreeNode *freeNodePtr = gFreeNodePtr; 

  // Loop through free node list until we find a large enough node, or reach end (NULL)
  while(true) { 
    if (freeNodePtr == NULL) { 
      printf("Error: allocMem: No free node available of size %d\n", size); 
      return NULL; 
    } 
    if (freeNodePtr->size >= size) { //found a free space that is large enough
      break; 
    } 
    // original global free node ptr beccomes the previous free node
    prevFreeNodePtr = freeNodePtr; 
    freeNodePtr = freeNodePtr->next; 
  } 
  
  FreeNode *newFreeNodePtr = NULL;
  //Explicitly checking if free node size is GREATER than (not equal to as this is handled differently below)
  if (freeNodePtr->size > (size + HEADER_SIZE)) { 

  	//if allocated region is less than free node, need to create a new free node
    FreeNode *nextFreeNodePtr = freeNodePtr->next; 

    //Type cast to char * first then back to FreeNode * to increment byte by byte (rather than jump to next struct section)
    newFreeNodePtr = (FreeNode *)(((char *)freeNodePtr) + size + HEADER_SIZE); 
    newFreeNodePtr->size = freeNodePtr->size - size - HEADER_SIZE; 
    newFreeNodePtr->next = nextFreeNodePtr;
  }

  //If there was a extra free space after allocated region 
  if (newFreeNodePtr != NULL) {
  	//First time, so update global ptr to new free node (after the allocated region)
    if (prevFreeNodePtr == NULL) { 
      gFreeNodePtr = newFreeNodePtr; 
    } else {
    	//If there is a previous free node, link its next prt to this newly created free block
      prevFreeNodePtr->next = newFreeNodePtr; 
    } 
  } else {
  	//If there was not extra free space (requested size + header size took up the entire free space)
  		//hence, no need to create a new free node, simply adjust free list pointers
    if (prevFreeNodePtr == NULL) { 
      gFreeNodePtr = freeNodePtr->next;
    } else {
      prevFreeNodePtr->next = freeNodePtr->next; 
    } 
  }

  //Allocated AFTER free node creation since we need freeNodePtr (unchanged)
  	//if we allocated before handling/updating free list, we would lose access to freeNodePtr
  AllocNode *allocNodePtr = (AllocNode *)freeNodePtr; 
  allocNodePtr->size = size; 
  allocNodePtr->magicNumber = MAGIC_NUMBER; 

  // return ptr to the START of usable allocated memory (ie. after the header ends)
  return (void *)((char *)freeNodePtr + HEADER_SIZE); 
} 

/* Frees the memory object that ptr points to. 
if ptr is NULL, then no operation is performed and 0 is returned. 
The function returns 0 on success, and -1 otherwise. */ 
int freeMem (void *ptr) { 
  if (ptr == NULL) { 
    return 0; 
  } 

  //first check if ptr is valid (if space has in fact been allocated) 
  AllocNode *allocNodePtr = (AllocNode *)((char *)ptr - HEADER_SIZE); 
  if (allocNodePtr->magicNumber != MAGIC_NUMBER) { 
    printf("Error: freeMem: not a pointer to alloc node\n"); 
    return -1; 
  } 

  //replace magic number with pointer to next free space 
  FreeNode *freeNodePtr = (FreeNode *)((char *)ptr - HEADER_SIZE); 
  freeNodePtr->next = gFreeNodePtr; 

  gFreeNodePtr = freeNodePtr; 
  //printf("freeMem: freed memory pointing to %p\n", ptr); 
  return 0; 
} 

/* Helper method for dumMem, prints free node content + formatting */
void printFreeNodes(FreeNode *freeNodePtr, int i) { 
  printf( 
    "Free Node %d: Size: %d, Next: %p\n", 
    i, freeNodePtr->size, freeNodePtr->next 
  ); 
  if (freeNodePtr->next != NULL) 
    printFreeNodes(freeNodePtr->next, i+1); 
} 

/* Method created for testing/tracking purposes
Calculates the total amount of free memory left, recursively */
int calcFreeMemory(FreeNode *freeNodePtr, int total) { 
  if (freeNodePtr == NULL)
    return total;
  else
    return calcFreeMemory(freeNodePtr->next, freeNodePtr->size + total);
} 

/* Debugging method, print out  */
void dumpMem() { 
  if (gFreeNodePtr != NULL) {
    printFreeNodes(gFreeNodePtr, 0); 
  }
  printf("gFreeNodePtr: %p\n", gFreeNodePtr);
  printf("Free memory: %d\n", calcFreeMemory(gFreeNodePtr, 0)); 
  printf("\n\n"); 
} 

/* Changed main method (used for testing) to testmain, as no longer needed or called) 
int testmain() { 
  printf("size of free node %ld\n", sizeof(FreeNode)); 
  printf("size of alloc node %ld\n", sizeof(AllocNode)); 

  void *memory = initMem(MEM_SIZE); 

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

  return 0; 
} */
