//Gabriela Pinto and Katherine Hansen
//2318655 and 2326665
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

#define PAYLOAD_SIZE 34
#define BUFFER_SIZE 100
#define MEMSIZE 64000

extern unsigned int ip_checksum(unsigned char *data, int length);
void *producer_function();
void *consumer_function();
sem_t *full =NULL;
sem_t *empty=NULL;

int flag = 0;

int nitems;
int buffer_size;

pthread_mutex_t mutex;


void sig_handler(int sig){
    printf("\nCtrlc found\n");
    printf("Exiting\n");
    flag = 1;
}

typedef struct {
  int item_no;          //number of the item produced
  unsigned short cksum;         //16-bit Internet checksum
  unsigned char payload[PAYLOAD_SIZE];      //random generated data
} item;



item *itemBuffer;

void *producer_function()
{
    int in=0;
    item next_produced;
    next_produced.item_no=0;
    int loop = 0;
    while(!flag){
      sem_wait(empty);
      pthread_mutex_lock(&mutex);

  //    next_produced.item_no++;

      for(int i=0;i<PAYLOAD_SIZE;i++){
        next_produced.payload[i]=(unsigned char) rand() %256;
      }

      next_produced.cksum=(unsigned short)ip_checksum(&next_produced.payload[0],PAYLOAD_SIZE);

      memcpy((void *)&itemBuffer[in],(void *)&next_produced,sizeof(item));
      next_produced.item_no++;
      // printf("next produced item num: %d\n",next_produced.item_no);


      in=(in+1)%nitems;
      pthread_mutex_unlock(&mutex);
      sem_post(full);
    }
    pthread_exit(0);
}

void *consumer_function()
{
  unsigned short cksum1,cksum2;
  item next_consumed;
  int counter=0;
  int out=0;

  while(!flag){
    sem_wait(full);
    pthread_mutex_lock(&mutex);

    memcpy((void *)&next_consumed, (void *)&itemBuffer[out], sizeof(item));

    if((counter!=(next_consumed.item_no))){
      printf("Item numbers are incorrect");
      printf("Exiting");
      exit(1);
    }

    counter++;

    out=(out+1)%nitems;

    cksum2=next_consumed.cksum;
    cksum1=(unsigned short)ip_checksum(&next_consumed.payload[0], PAYLOAD_SIZE);

    if(cksum1!=cksum2){
      printf("checksum mismatch: received 0x%x, expected 0x%x \n",cksum2,cksum1);
    }

    sem_post(empty);
    pthread_mutex_unlock(&mutex);

  }
  //printf("checksum2:0x%x , checksum1:0x%x\n",cksum2,cksum1);
  pthread_exit(0);
}

int main(int argc, char *argv[])
{
  pthread_t producer,consumer;

  int status;//for error checking
  pthread_attr_t attr;

  int ID[5] = {0,1,2}; //Just used for numbering the producer and consumer
  struct sigaction act;
  act.sa_handler=sig_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags=0;
  sigaction(SIGINT, &act, 0);
    if (argc != 2) {
        printf("Usage: %s <nitems> \n", argv[1]);
        return -1;
    }

    nitems = atoi(argv[1]);
    if(nitems*40>MEMSIZE || nitems<=0){
      printf("Too many items\n");
      return -1;
    }


    buffer_size=nitems*40;
    itemBuffer = malloc(BUFFER_SIZE);
    //opening all 3 semaphors

    sem_unlink("full");
    sem_unlink("empty");

    full = sem_open("full", O_CREAT | O_EXCL, 0644, 0);
    if(full == NULL) {
      perror("Semaphore initialization failed");
      return -1;
    }
    empty = sem_open("empty", O_CREAT | O_EXCL, 0644, nitems);
    if(empty == NULL) {
      perror("Semaphore initialization failed");
      return -1;
    }

    sem_unlink("full");
    sem_unlink("empty");
    //opening Mutex
    pthread_mutex_init(&mutex, NULL);

    //getting default attributes
    pthread_attr_init(&attr);

    //producer funct
    pthread_create(&producer, &attr, (void *)producer_function, (void *)&ID[0]);
    //consumer funct
    pthread_create(&consumer, &attr, (void *)consumer_function, (void *)&ID[1]);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);


    pthread_mutex_destroy(&mutex);
    printf("finish\n");
    free(itemBuffer);
    return 0;
}
