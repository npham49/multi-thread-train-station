#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/queue.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
//--------------------------------------------------------------------------------------------
//Queue code snipped cited from https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/
struct Queue {
    int front, rear, size;
    unsigned capacity;
    int* array;
};
 
// function to create a queue
// of given capacity.
// It initializes size of queue as 0
struct Queue* createQueue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*)malloc(
        sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
 
    // This is important, see the enqueue
    queue->rear = capacity - 1;
    queue->array = (int*)malloc(
        queue->capacity * sizeof(int));
    return queue;
}

// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
    return (queue->size == 0);
}
 
// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, int item)
{
    queue->rear = (queue->rear + 1)
                  % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    //printf("%d enqueued to queue\n", item);
}
 
// Function to remove an item from queue.
// It changes front and size
int dequeue(struct Queue* queue)
{
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1)
                   % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}
 
//--------------------------------------- 

struct Train{
    pthread_cond_t go;
    int train_no;
    int load_time;
    int cross_time;
    char direction;
    int permission;
    char prdir[4];

}*newTrain;
//---------------------------------------------------------------------------------------------------------------
struct Train *trainsar[50];
struct Queue* low;
struct Queue* high;
pthread_mutex_t start_timer= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t train_ready_to_load=PTHREAD_COND_INITIALIZER;

pthread_mutex_t addtoqueue= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t track = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dispatch= PTHREAD_COND_INITIALIZER;

long t_s_s = 0;
long t_s_ns = 0; 
float t_s = 0.0; 

void *createTrain(void* train) {
    struct Train *self = (struct Train*)train;
    pthread_cond_init(&self->go, NULL);

    struct timespec time1 = {0};
    struct timespec time2 = {0};

    pthread_mutex_lock(&start_timer);
    pthread_cond_wait(&train_ready_to_load, &start_timer);
    pthread_mutex_unlock(&start_timer);

    //getting the direction string for printing
    if(strcmp(&self->direction, "e")==0){
        strcpy(self->prdir,"East");
    } else if (strcmp(&self->direction, "E")==0)
    {
        strcpy(self->prdir,"East");
    } else if (strcmp(&self->direction, "w")==0)
    {
        strcpy(self->prdir,"West");
    } else if (strcmp(&self->direction, "W")==0)
    {
        strcpy(self->prdir,"West");
    }

    //start the loading process
    usleep(self -> load_time*100000);

    //get the timestamp when the program finishes loading
    clock_gettime(CLOCK_MONOTONIC, &time2);
    long t_t_s2 = time2.tv_sec;
    long t_t_ns2 = time2.tv_nsec;
    float t_t2 = t_t_s2+(float)t_t_ns2/1000000000;
    printf("00:00:%04.1lf Train %d is ready to go %s\n",t_t2-t_s-1.0000, self -> train_no, self -> prdir);

    //lock mutex for adding to queues
    pthread_mutex_lock(&addtoqueue);

    //adding the train to the appropriate queue
    if(strcmp(&self->direction, "e")==0){
        enqueue(low,self->train_no);
    } else if (strcmp(&self->direction, "E")==0)
    {
        enqueue(high,self->train_no);
    } else if (strcmp(&self->direction, "w")==0)
    {
        enqueue(low,self->train_no);
    } else if (strcmp(&self->direction, "W")==0)
    {
        enqueue(high,self->train_no);
    }
    pthread_mutex_unlock(&addtoqueue);

    pthread_mutex_lock(&track);
    //wait for the signal from diapatcher to run on track
    while(self->permission!=1) {
        pthread_cond_wait(&(self->go), &track);
    }
    //get timestamp of train going on track
    clock_gettime(CLOCK_MONOTONIC, &time2);
    t_t_s2 = time2.tv_sec;
    t_t_ns2 = time2.tv_nsec;
    t_t2 = t_t_s2+(float)t_t_ns2/1000000000;
    printf("00:00:%04.1lf Train %d is ON the main track going %s\n",t_t2-t_s-1.0000, self -> train_no, self -> prdir);

    //train is crossing
    usleep(self -> cross_time*100000);

    //get timestamp of train finishing the crossing
    clock_gettime(CLOCK_MONOTONIC, &time2);
    t_t_s2 = time2.tv_sec;
    t_t_ns2 = time2.tv_nsec;
    t_t2 = t_t_s2+(float)t_t_ns2/1000000000;
    printf("00:00:%04.1lf Train %d is OFF the main track after going %s\n",t_t2-t_s-1.0000, self -> train_no, self -> prdir);

    //signalling the dispatch to move on to the next element in the queue
    pthread_cond_signal(&dispatch);
    pthread_mutex_unlock(&track);

    pthread_exit(0);
}

void * dispatcherf(void* remaining) {
    //making a copy of the remaining trains count
    int count = *((int *) remaining);
    int trainsleft=count;
    while(trainsleft>0) {
        pthread_mutex_lock(&track);
        //empty the high priority queue first and send all trains on track one at a time
        while(isEmpty(high)==0) {
            int tobedis=dequeue(high);
            trainsar[tobedis]->permission=1;
            pthread_cond_signal(&(trainsar[tobedis] -> go));
            pthread_cond_wait(&dispatch, &track);
            trainsleft--;
        }
        //empty the low priority queue and send all trains on track one at a time
        while(isEmpty(low)==0) {
            int tobedis=dequeue(low);
            trainsar[tobedis]->permission=1;
            pthread_cond_signal(&(trainsar[tobedis] -> go));
            pthread_cond_wait(&dispatch, &track);
            trainsleft--;
        }

        pthread_mutex_unlock(&track);
    }
    pthread_exit(0);
}

int main(int argc,char **argv) {
    int count=0;
    char *train;
    char buff[90];
    char const* const fileName = argv[1];
    FILE* file = fopen(fileName, "r");
    char line[256];
    //2 queues for low priority trains and high priority trains
    low = createQueue(100);
    high = createQueue(100);

    pthread_t trains[50];
    pthread_t dispatcher;
    struct timespec time0 = {0};

    //getting the timestamp for simulation start
    clock_gettime(CLOCK_MONOTONIC, &time0);
	t_s_s = time0.tv_sec;
	t_s_ns = time0.tv_nsec;
    t_s = (float)t_s_s+(float)t_s_ns/1000000000;
    
    //get infomations from input and add to the train array and start a new thread with every array element
    while (fgets(line, sizeof(line), file)) {
        train = (char*)malloc(10 * sizeof(char));
        sprintf(buff, "%d ", count);
        strcat(buff,line);
        strcpy(train,buff);

        int train_no=atoi(strtok(train," "));
        char *direction = strtok(NULL," ");
        newTrain = (struct Train *) malloc(sizeof(struct Train));
        newTrain->train_no=train_no;
        newTrain->load_time=atoi(strtok(NULL," "));
        newTrain->cross_time=atoi(strtok(NULL," "));
        newTrain->direction=*direction;
        newTrain->permission=0;
        if(pthread_create(&trains[count], NULL, &createTrain, (void *) newTrain)) {
            perror("pthread create failed");
            exit(EXIT_FAILURE);
        }
        trainsar[train_no] = newTrain;
        count++;
    }

    sleep(1);
    
    pthread_cond_broadcast(&train_ready_to_load);

    if(pthread_create(&dispatcher, NULL, &dispatcherf, (void *) &count))
    {
        perror("pthread create failed");
        exit(EXIT_FAILURE);
    }
    pthread_join(dispatcher, NULL);
    
    fclose(file);
    sleep(1);
    free(train);
    for (int i = 0; i < count; i++) {
        free(trainsar[i]);
    }
    return 0;
}