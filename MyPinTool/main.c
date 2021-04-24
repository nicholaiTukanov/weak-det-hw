#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

void *functionC();
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main()
{
   pthread_t thread1, thread2, thread3, thread4;
   int rc1 = pthread_create( &thread1, NULL, &functionC, NULL);
   int rc2 = pthread_create( &thread2, NULL, &functionC, NULL);
   int rc3 = pthread_create( &thread3, NULL, &functionC, NULL);
   int rc4 = pthread_create( &thread4, NULL, &functionC, NULL);

   // pthread_join( thread1, NULL);
   // pthread_join( thread2, NULL);
   // pthread_join( thread3, NULL);
   // pthread_join( thread4, NULL);
   return 0;
}

void *functionC()
{
   pthread_mutex_lock( &mutex1 );
   counter++;
   // printf("Thread id: %ld ", pthread_self());
   printf("Counter value: %d\n",counter);
   pthread_mutex_unlock( &mutex1 );

   return NULL;
}

