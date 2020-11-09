# thread_synch

### Gabriela Pinto and Katherine Hansen
### 2318655 2326665
#### running instructions:
- gcc -c ip_checksum.c
- gcc prodcon.c -o prodcon ip_checksum.o
- To exit: ctrl-c


#### Known Errors--fixed
- We could not get the item number validation to work
- we fixed this errors by including the sem_unlink before the semaphors were created
