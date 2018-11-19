#include <stdio.h>
#include "queue.h"

int main() {
  queue_t* c = createQueue();
  int a[10] = {0,1,2,3,4,5,6,7,8,9};
  int i;
  for (i=0; i<10; i++) {
    enqueue(c, &a[i]);
  }

  for (i=0; i<10; i++) {
    int* e = dequeue(c);
    printf("dequeued %d\n", *e);
  }
  return 0;
}
