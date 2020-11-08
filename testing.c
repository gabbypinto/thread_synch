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

sem_t *full =NULL;
sem_t *empty=NULL;
int in = 0;
int out = 0;
int nitems;
int buffer_size;

pthread_mutex_t mutex;

unsigned char *buffer;

void sig_handler(int sig){
    printf("\nCtrlc found\n");
    /* remove the shared memory object */
    printf("Exiting\n");
    exit(1);
}

typedef struct {
  int item_no;          //number of the item produced
  unsigned short cksum;         //16-bit Internet checksum
  unsigned char payload[PAYLOAD_SIZE];      //random generated data
} item;

item *itemBuffer;

void *producer_function()
{
    item next_produced;
    unsigned short cksum, cksum1, cksum2;

    printf("producer func\n");

    for(int i = 0; i < MEMSIZE; i++) {
      //  itemBuffer = rand(); // Produce an random item
        sem_wait(empty);
        pthread_mutex_lock(&mutex);
        next_produced.item_no++;
        for(i=0;i<nitems;i++){
          //3. generate the payload data
          next_produced.payload[i] = (unsigned char) rand() % 256;
        }
        next_produced.cksum = (unsigned short) ip_checksum(&next_produced.payload[0],PAYLOAD_SIZE);
        memcpy((void *)&itemBuffer[in],&next_produced,sizeof(next_produced));

        itemBuffer[in] = next_produced;
        //printf("Producer %d: Insert Item %d at %d\n", *((int *)pno),itemBuffer[in],in);
        in = (in+1)%BUFFER_SIZE;

        pthread_mutex_unlock(&mutex);
        sem_post(full);
    }
    return NULL;
}
void *consumer_function()
{
    item next_consumed;
    unsigned short cksum1, cksum2;
    int bufferCount = 0;
    printf("consumer func\n");

    for(int i = 0; i < MEMSIZE; i++) {
        sem_wait(full);
        pthread_mutex_lock(&mutex);

        printf("seg?? \n");
        memcpy(&next_consumed, &buffer[out],sizeof(item)); //<-seg here
        next_consumed = itemBuffer[out];
      //  printf("Consumer %d: Remove Item %d from %d\n",*((int *)cno),item, out);
        out = (out+1)%BUFFER_SIZE;
        cksum1 = (unsigned short )ip_checksum (&buffer[0],PAYLOAD_SIZE); //<-seg here

        if(cksum1!=cksum2){
          printf("checksum mismatch: received 0x%x, expected 0x%x \n",cksum2,cksum1);
          break;
        }
        else{
          printf("we good");
        }
        bufferCount++;
        next_consumed.item_no++;
        pthread_mutex_unlock(&mutex);
        sem_post(empty);
    }
    return NULL;
}

int main(int argc, char *argv[])
{

    pthread_t producer,consumer;
    pthread_mutex_init(&mutex, NULL);
    // sem_init(&empty,0,BufferSize);
    // sem_init(&full,0,0);
    full = sem_open("full", O_CREAT | O_EXCL, 0644, 0);
    empty = sem_open("empty", O_CREAT | O_EXCL, 0644, 0);


    int ID[5] = {0,1}; //Just used for numbering the producer and consumer

    printf("hello\n");

    if (argc != 2) {
        printf("Usage: %s <nitems> \n", argv[1]);
        return -1;
    }

    nitems = atoi(argv[1]);
    printf("nitem %d\n",nitems);
    printf("after items\n");
    if(nitems*40>MEMSIZE || nitems<=0){
      printf("Too many items");
      return -1;
    }

    printf("HI\n");
    buffer_size=nitems*40;
    itemBuffer = malloc(BUFFER_SIZE);
    //
    pthread_create(&producer, NULL, (void *)producer_function, (void *)&ID[0]);
    pthread_create(&consumer, NULL, (void *)consumer_function, (void *)&ID[1]);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    // pthread_mutex_destroy(&mutex);
    // sem_destroy(&empty);
    // sem_destroy(&full);

    return 0;
}
