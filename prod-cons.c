/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.	
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#define QUEUESIZE 10
#define LOOP 20000
#define MAXCONSUMERS 100

void *producer (void *args);
void *consumer (void *args);

typedef struct{
  void * (*work)(void *);
  void * arg;
  struct timeval startTime;
} workFunction;

typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail, ended;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  int* times;
  int outCounter;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

int p = 3;
int q;

int main ()
{
  queue *fifo;
  FILE *fp;
  // Open file
  fp = fopen("stats.txt", "w");

  printf("Testing for 1 to %d consumers...\n", MAXCONSUMERS);

  for (q=1;q<MAXCONSUMERS+1;q++){

    pthread_t pro[p];
    pthread_t con[q];

    fifo = queueInit ();
    if (fifo ==  NULL) {
      exit (1);
    }

    // Start producer threads
    for (int i=0;i<p;i++) {
      pthread_create (&pro[i], NULL, producer, fifo);
    }
    // Start producer threads
    for (int i=0;i<q;i++) {
      pthread_create (&con[i], NULL, consumer, fifo);
    }

    // Wait for producer threads
    for (int i=0;i<p;i++) {
      pthread_join (pro[i], NULL);
    }
    // Wait for producer threads
    for (int i=0;i<q;i++) {
      pthread_join (con[i], NULL);
    }

    for (int i=0;i<fifo->outCounter;i++) {
      fprintf(fp, "%d,", fifo->times[i]);
    }
    fprintf(fp, "\n");

    queueDelete (fifo);

  }

  // Close file.
  fclose(fp);

  return 0;
}

void *work (void* args)
{
  int* rad = (int*) args;
  
  for(int i=0;i<10;i++){
    sin((double) (*rad+i));
  }
}

void *producer (void *q)
{
  queue *fifo;
  int i;
  int rad = 5;

  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++) {

    //Create work function structure
    workFunction inFunc;
    inFunc.work = &work;
    inFunc.arg = &rad;
    gettimeofday(&inFunc.startTime, NULL);

    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    queueAdd (fifo, inFunc);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }

  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  workFunction outFunc;
  struct timeval endTime;

  fifo = (queue *)q;

  while (1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    if (fifo->ended) {
      // If all work functions have ended break while loop and inform the next thread
      pthread_mutex_unlock (fifo->mut);
      pthread_cond_signal (fifo->notEmpty);
      break;
    }

    queueDel (fifo, &outFunc);
    gettimeofday(&endTime, NULL);
    int waitTime = (int) ((endTime.tv_sec - outFunc.startTime.tv_sec) * 1e6 + (endTime.tv_usec - outFunc.startTime.tv_usec));
    fifo->times[fifo->outCounter++] = waitTime;
    
    if (fifo->outCounter == p * LOOP) {
      // If this thread does the last work function, then break and infrom all other threads that queue functions ended
      fifo->ended = 1;
      fifo->empty = 0;
      pthread_mutex_unlock (fifo->mut);
      pthread_cond_signal (fifo->notEmpty);
      break;
    }

    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);

    outFunc.work(outFunc.arg);
  }

  return (NULL);
}

queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->ended = 0;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);

  q->times = (int*) malloc(p * LOOP * sizeof(int));
  q->outCounter = 0;
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q->times);
  free (q);
}

void queueAdd (queue *q, workFunction in)
{
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
