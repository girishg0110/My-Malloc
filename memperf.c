#include "mymalloc.c"

#include <sys/time.h>

#define NUM_OPS (1<<20)
#define NUM_PTRS (1<<12)

long int getSeconds() {
   struct timeval* randomSeed = (struct timeval*) malloc(sizeof(struct timeval));
   gettimeofday(randomSeed, NULL);
   long int seconds = randomSeed->tv_sec;
   long int microseconds = randomSeed->tv_usec;
   free(randomSeed);
   return seconds * 1000000 + microseconds;
}

void setSeed() {
   srand(getSeconds());
}

int randRange(int low, int high) {
   return rand() % (high-low+1) + low;
}

void printMetrics(int alg, double throughput, double utilization) {
   if (alg == FIRSTFIT) printf("First");
   else if (alg == NEXTFIT) printf("Next");
   else if (alg == BESTFIT) printf("Best");
   printf(" fit throughput: %f ops/sec\n", throughput);

   if (alg == FIRSTFIT) printf("First");
   else if (alg == NEXTFIT) printf("Next");
   else if (alg == BESTFIT) printf("Best");
   printf(" fit utilization: %.2f\n", utilization);
}

void runTests(int alg) {
   myinit(alg);

   void* pointers[NUM_PTRS];
   for (int i = 0; i < NUM_PTRS; i++) {
      pointers[i] = NULL;
   }
   long int startTime = getSeconds();
   for (int i = 0; i < NUM_OPS; ) {
      // Choose an operation, target/parameters
     int target = randRange(0, NUM_PTRS-1);
      if (pointers[target]) {
         int op = randRange(0, 1);
         if (op == 0) { 
            // free
            myfree(pointers[target]);
            pointers[target] = NULL;
            i++;
         } else if (op == 1) {
            // realloc
            size_t resize = (size_t) randRange(1, 256);
            void* ptr = myrealloc(pointers[target], resize);
            if (ptr) {
               pointers[target] = ptr;
               i++;
            }
         }
      } else {
         // malloc
         size_t size = (size_t) randRange(1, 256);
         void* ptr = mymalloc(size);
         if (ptr) {
            pointers[target] = ptr;
            i++;
         }
      }
   }
   long int endTime = getSeconds();
   
   long int elapsedTime = endTime - startTime;

   double throughput = 1000000.0 * NUM_OPS/elapsedTime;
   printMetrics(alg, throughput, utilization());

   mycleanup();
}

int main() {
   int algs[] = {FIRSTFIT, NEXTFIT, BESTFIT};
   setSeed();

   for (int i = 0; i < 3; i++) {
      runTests(algs[i]);
   } 
}
