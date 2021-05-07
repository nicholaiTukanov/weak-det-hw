#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

void *functionC();
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main()
{
   // pthread_t thread1, thread2, thread3, thread4, thread5, thread6, thread7, thread8;

   int N = 100000;
   pthread_t *threads = malloc(sizeof(pthread_t) * N);

   for (int i=0; i<N; i++) {
      pthread_t thread = threads[i];
      pthread_create(&thread, NULL, &functionC, 0);

      pthread_join(thread, NULL);
   }

   printf("FINAL COUNT: %d\n", counter);
   return 0;
}

void *functionC(void *vargp)
{
   
   pthread_mutex_lock( &mutex1 );
   
   counter++;

   pthread_mutex_unlock( &mutex1 );

   return NULL;
}

