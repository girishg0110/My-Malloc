#include "mymalloc.h"

#define MAX_MALLOC_SIZE (1<<20)
#define HEADER_SIZE (sizeof(Header))
#define FOOTER_SIZE (sizeof(Footer))

#define NOT_ALLOC 0
#define ALLOC 1

/* Struct and Global Variables */

typedef struct Header {
   size_t requestedSize; 
   size_t size;
   struct Header* next;
   struct Header* prev;
} Header; 

typedef struct Footer {
   size_t size;
} Footer;

char* space;
int allocAlgorithm;
Header* freeHead;
Header* nextFitTail;

/* Helper Methods */ 

Header* nextBlock(Header* curr) {
   if (curr == NULL) return NULL;
   Header* nextHeader = (Header*) ((void*) (curr + 1) + (curr->size & -2) + FOOTER_SIZE);
   if ((((size_t) nextHeader) >= ((size_t) space) + MAX_MALLOC_SIZE)) return NULL;
   return nextHeader;
}

Header* previousBlock(Header* curr) {
   if (curr == NULL) return NULL;
   Footer* prevFoot = (Footer*) ((void*)curr - FOOTER_SIZE);
   if ((size_t) prevFoot < (size_t) space) return NULL;
   Header* prevHead = (Header*) ((void*)prevFoot - (prevFoot->size & -2) - HEADER_SIZE);
   return prevHead;
}

Footer* getFooter(Header* curr) {
   return ((Footer*) ((void*) (curr+1) + (curr->size & -2)));
}

size_t addPadding(size_t size) {
   return (size % 8 == 0) ? size : size + (8 - size % 8);
}

Header* getFreeBlock(size_t size) {
   if (allocAlgorithm == FIRSTFIT) {
      Header* ptr = freeHead;
      while (ptr) { 
         if ((ptr->size == size) || (ptr->size >= HEADER_SIZE + size + FOOTER_SIZE)) {
            return ptr;
         } else {
            if (ptr != ptr->next) ptr = ptr->next;
         }
      }
   } else if (allocAlgorithm == NEXTFIT) {
      Header* ptr = nextFitTail;
      while (ptr) {
         if ((ptr->size == size) || (ptr->size >= HEADER_SIZE + size + FOOTER_SIZE)) {
            return ptr;
         } else {
            ptr = ptr->next;
         }
      }
      ptr = freeHead;
      while (ptr && (ptr != nextFitTail)) {
         if ((ptr->size == size) || (ptr->size >= HEADER_SIZE + size + FOOTER_SIZE)) {
            return ptr;
         } else {
            ptr = ptr->next;
         }
      }
   } else if (allocAlgorithm == BESTFIT) {
      Header* ptr = freeHead;
      Header* bestFit = NULL;
      while (ptr) {
         if ((ptr->size == size) || (ptr->size >= HEADER_SIZE + size + FOOTER_SIZE)) {
            if (!bestFit || (bestFit->size > ptr->size)) bestFit = ptr;
         }
         ptr = ptr->next;
      }
      if (bestFit) return bestFit;
   }

   return NULL;
}

double utilization() {
   Header* lastFree = (Header*) space;
   size_t spaceRequested = 0;
   for (Header* ptr = (Header*) space; ptr && ((size_t) ptr < (size_t) (space + MAX_MALLOC_SIZE)); ptr = nextBlock(ptr)) {
      if ((ptr->size & 1) == NOT_ALLOC) lastFree = ptr;
      else spaceRequested += ptr->requestedSize;
   }
   
   size_t spaceUsed = (size_t) lastFree - (size_t) space;
   double utilRatio = (spaceUsed != 0) ? 1.0 * spaceRequested / spaceUsed : 1.0;

   return utilRatio;
}


/* Display Methods */

void printHeader(Header* header) {
   printf(
      "Address %ld: requestedSize %ld, size %ld, alloc bit %ld, prev %ld, next %ld, footer %ld\n", (size_t) header, header->requestedSize,       header->size & -2, header->size & 1, (size_t) header->prev, (size_t) header->next, getFooter(header)->size
   );
}

void printFreeList() {
   Header* ptr = freeHead;
   while (ptr) {
      printHeader(ptr);
      ptr = ptr->next;
   }
   printf("---END OF FREE LIST---\n\n");
}

void printAllBlocks() {
   for (Header* ptr = (Header*) space; ptr && ((size_t) ptr < (size_t) space + MAX_MALLOC_SIZE); ptr = nextBlock(ptr)) {
      printHeader(ptr);
   }
   printf("Free head at %ld\n", (size_t) freeHead);
   if (allocAlgorithm == NEXTFIT) printf("Next fit tail at %ld\n", (size_t) nextFitTail);
   printf("\n");
}

/* Memory Management */

void myinit(int allocArg) {
   allocAlgorithm = allocArg;
   space = (char*) malloc(MAX_MALLOC_SIZE);

   // Initialize explicit free list
   //    Stores: size & free/alloc (0/1), next ptr, prev ptr
   int i = 0; 
   for (size_t* ptr = (size_t*) space; i < MAX_MALLOC_SIZE; ptr++, i += sizeof(size_t)) { 
      *ptr = -1; 
   }
   freeHead = (Header*) space;
   freeHead->requestedSize = 0;
   freeHead->size = MAX_MALLOC_SIZE - HEADER_SIZE - FOOTER_SIZE;
   freeHead->next = NULL;
   freeHead->prev = NULL;

   Footer* firstFooter = getFooter(freeHead);
   firstFooter->size = freeHead->size;

   /*
   printf("Heap Size: %d\n", MAX_MALLOC_SIZE);
   printf("Header Size: %ld\n", HEADER_SIZE);
   printf("Footer Size: %ld\n", FOOTER_SIZE);
   printf("First free block size: %ld\n\n", freeHead->size);
   */

   nextFitTail = freeHead;
}

void* mymalloc(size_t size) {
   if (size == 0) return NULL; 
   size_t paddedSize = addPadding(size);
   Header* freeBlock = getFreeBlock(paddedSize);
   if (freeBlock) {
      // Save old metadata
      size_t oldSize = freeBlock->size;
      Header* oldNext = freeBlock->next;
      Header* oldPrev = freeBlock->prev;

      // Write allocated metadata
      freeBlock->requestedSize = size;
      freeBlock->size = paddedSize | ALLOC;
      freeBlock->next = NULL;
      freeBlock->prev = NULL;
      
      // Start of user region
      void* startOfFree = (void*) (freeBlock + 1);
      Footer* newFooter = getFooter(freeBlock);
      newFooter->size = paddedSize | ALLOC;  

      // Splitting
      if (paddedSize != oldSize) {
         Header* splitFreeBlock = nextBlock(freeBlock);
         splitFreeBlock->requestedSize = 0;
         splitFreeBlock->size = oldSize - HEADER_SIZE - FOOTER_SIZE - paddedSize;
         splitFreeBlock->next = oldNext;
         splitFreeBlock->prev = oldPrev;
         Footer* splitFooter = getFooter(splitFreeBlock);
         splitFooter->size = splitFreeBlock->size;

         if (oldPrev) oldPrev->next = splitFreeBlock;
         else freeHead = splitFreeBlock;
         if (oldNext) oldNext->prev = splitFreeBlock;
         nextFitTail = splitFreeBlock;
      } else {
         if (oldPrev) oldPrev->next = oldNext;
         else freeHead = oldNext;
         if (oldNext) oldNext->prev = oldPrev;
         nextFitTail = freeHead;
      }

      return startOfFree;
   }
   return NULL;
}

void myfree(void* ptr) {
   if (ptr == NULL) return;
   if (((size_t) ptr < (size_t) space) || ((size_t) ptr > (size_t) (space + MAX_MALLOC_SIZE) )) {
      printf("error: not a heap pointer\n");
   }
   for (Header* blockPtr = (Header*) space; (size_t) blockPtr < (size_t) (space + MAX_MALLOC_SIZE); blockPtr = nextBlock(blockPtr)) {
      if (blockPtr + 1 == ptr) {
         if ((blockPtr->size & 1) == NOT_ALLOC) {
            printf("error: double free\n");
         } else {
            // Mark as unallocated
            blockPtr->requestedSize = 0;
            blockPtr->size &= -2;
            Footer* ptrFooter = getFooter(blockPtr);
            ptrFooter->size = blockPtr->size;

            // Coalescing
            Header* blockAfter = nextBlock(blockPtr);
            if (blockAfter) {
               if (((blockAfter->size & 1) == NOT_ALLOC)) {
                  if (blockAfter == nextFitTail) nextFitTail = blockPtr;

                  Header* next = blockAfter->next;
                  Header* prev = blockAfter->prev;                  
                  if (next) next->prev = prev;
                  if (prev) prev->next = next;
                  else freeHead = next;
         
                  blockPtr->size += blockAfter->size + HEADER_SIZE + FOOTER_SIZE;
                  Footer* afterFooter = getFooter(blockPtr);
                  afterFooter->size = blockPtr->size;
               }
            }
            
            Header* blockBefore = previousBlock(blockPtr);
            if (blockBefore) {
               if ((blockBefore->size & 1) == NOT_ALLOC) {
                  if ((blockPtr == nextFitTail) || (blockBefore == nextFitTail)) nextFitTail = blockBefore;          

                  Header* next = blockBefore->next;
                  Header* prev = blockBefore->prev;
                  if (next) next->prev = prev;
                  if (prev) prev->next = next;
                  else freeHead = next; 
 
                  blockBefore->size += blockPtr->size + HEADER_SIZE + FOOTER_SIZE;
                  Footer* beforeFooter = getFooter(blockBefore);
                  beforeFooter->size = blockBefore->size;

                  blockPtr = blockBefore;
               }
            }

            blockPtr->prev = NULL;
            blockPtr->next = freeHead;
            if (freeHead) freeHead->prev = blockPtr;
            freeHead = blockPtr;
         }
         
         return;
      }
   } 
   printf("error: not a malloced address\n");
}

void* myrealloc(void* ptr, size_t size) {
   if (!ptr && !size) return NULL;
   else if (!ptr) return mymalloc(size);
   else if (!size) { myfree(ptr); return NULL; }

   size_t paddedSize = addPadding(size);
   Header* blockPtr = (Header*) (ptr - HEADER_SIZE);
   size_t currentSize = blockPtr->size & -2;

   if (currentSize >= paddedSize) {
      return ptr;
   } else {
      Header* blockNext = nextBlock(blockPtr);
      if (blockNext && ((blockNext->size & 1) == NOT_ALLOC) && (currentSize + (blockNext->size & -2) >= paddedSize)) {
         size_t nextSize = blockNext->size & -2;
         if (blockNext == nextFitTail) {
            if (nextFitTail->next) nextFitTail = nextFitTail->next;
            else nextFitTail = freeHead;
         } 

         // INPLACE
         Header* prev = blockNext->prev;
         Header* next = blockNext->next;
      
         // Update size and footer of blockPtr
         blockPtr->requestedSize = size;
         blockPtr->size = paddedSize | ALLOC;
         Footer* ptrFooter = getFooter(blockPtr);
         ptrFooter->size = blockPtr->size;
         
         // Create new free block and add to free list
         Header* newFree = nextBlock(blockPtr);
         newFree->requestedSize = 0;
         newFree->size = currentSize + nextSize - paddedSize;
         newFree->size &= -2;
         newFree->next = next;
         newFree->prev = prev;
         Footer* newFooter = getFooter(newFree);
         newFooter->size = newFree->size;

         if (prev) prev->next = newFree;
         else freeHead = newFree;
         if (next) next->prev = newFree;

         return ptr;
      } else {
         // RELOCATE
         char* newBlock = (char*) mymalloc(size);
         if (newBlock) {
            int i = 0;
            for (char* x = (char*)(blockPtr + 1); i < blockPtr->requestedSize; i++, x++) {
               newBlock[i] = *x;
            }
         } else {
            return NULL;
         }
         myfree(ptr);
         
         return newBlock;
      }
   }

   return NULL;
}

void mycleanup() {
   free(space);
   freeHead = NULL;
   nextFitTail = NULL;
}
