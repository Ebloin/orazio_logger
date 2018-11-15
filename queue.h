#include <stdlib.h>

//Struttura del nodo della coda
struct node {
  void* data;
  struct node* next;
};
//Struttura della coda che contiene primo e ultimo elemento
typedef struct queue {
  struct node* head;
  struct node* tail;
} queue_t;

//Crea un nuovo nodo
struct node* newNode(void* data) {
  struct node* temp= (struct node*) malloc(sizeof(struct node));
  temp->data = data;
  temp->next = NULL;
  return temp;
}

//Crea una nuova coda
struct queue* createQueue() {
  struct queue* q = (struct queue*) malloc(sizeof(struct queue));
  q->head = NULL;
  q->tail = NULL;
  return q;
}

//Aggiunge in coda un nuovo nodo con le informazioni contenute in data
void enqueue(struct queue* q, void* data) {
  struct node* temp = newNode(data);
  if (q->tail == NULL) {
    q->head = q->tail = temp;
    return;
  }
  q->tail->next = temp;
  q->tail = temp;
}

//Consuma dalla coda e ritorna il valore contenuto nel campo data del primo elemento
void* dequeue(struct queue* q) {
  if (q->head == NULL) {
    return NULL;
  }
  struct node* temp = q->head;
  q->head = q->head->next;
  if (q->head == NULL) {
    q->tail = NULL;
  }
  return temp->data;
}
