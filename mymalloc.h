#include <stdlib.h>
#include <stdio.h>

#define FIRSTFIT 0
#define NEXTFIT 1
#define BESTFIT 2

void myinit(int allocArg);
void* mymalloc(size_t size);
void myfree(void *ptr);
void* myrealloc(void *ptr, size_t size);
void mycleanup();
double utilization();  

