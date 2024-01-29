#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

#define GEN_TO_SCH_KEY 1564

#define NPROC 10 //Maximum number of processes

#define CLKDURATION 1

///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================



int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(CLKDURATION);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
struct process {
    int id;
    int arrivalTime;
    int runTime;
    int priority;
} typedef process;

enum algorithm {
  SJF,
  SRTF,
  RR
} typedef algorithm; 

struct msgbuff {
  long mtype;
  process mprocess;
} typedef msgbuff;

typedef struct PCB {
    int id;
    pid_t pid;
    int arrivalTime;
    int runTime;
    int priority;
    int startTime;
    int endTime;
    int remainingTime;
    int waitingTime;
    int turnaroundTime;
    double weightedturnaroundTime;

} PCB;


// C program for array implementation of queue
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// A structure to represent a queue
struct readyQueue {
	int front, rear, size;
	unsigned capacity;
	int* array;
};

// function to create a queue
// of given capacity.
// It initializes size of queue as 0
struct readyQueue* createQueue(unsigned capacity)
{
	struct readyQueue* queue = (struct readyQueue*)malloc(
		sizeof(struct readyQueue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;

	// This is important, see the enqueue
	queue->rear = capacity - 1;
	queue->array = (int*)malloc(
		queue->capacity * sizeof(int));
	return queue;
}

// readyQueue is full when size becomes
// equal to the capacity
// readyQueue is full when size becomes
// equal to the capacity minus 1
int isFull(struct readyQueue* queue)
{
    return (queue->size == (queue->capacity - 1));
}

// readyQueue is empty when size is 0
int isEmpty(struct readyQueue* queue)
{
	return (queue->size == 0);
}

// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct readyQueue* queue, int item)
{
	if (isFull(queue))
		return;
	queue->rear = (queue->rear + 1)
				% queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
	//printf("%d enqueued to queue\n", item);
}

// Function to remove an item from queue.
// It changes front and size
int dequeue(struct readyQueue* queue)
{
	if (isEmpty(queue))
		return INT_MIN;
	int item = queue->array[queue->front];
	queue->front = (queue->front + 1)
				% queue->capacity;
	queue->size = queue->size - 1;
	return item;
}

// Function to get front of queue
int front(struct readyQueue* queue)
{
	if (isEmpty(queue))
		return INT_MIN;
	return queue->array[queue->front];
}

// Function to get rear of queue
int rear(struct readyQueue* queue)
{
	if (isEmpty(queue))
		return INT_MIN;
	return queue->array[queue->rear];
}

//** PQ Implementation**


// Node
typedef struct node {
    int data;

    // Lower values indicate higher priority
    int priority;

    struct node* next;

} Node;

// Function to Create A New Node
Node* newNode(int d, int p)
{
    Node* temp = (Node*)malloc(sizeof(Node));
    if (temp != NULL) {
        temp->data = d;
        temp->priority = p;
        temp->next = NULL;
    }
    return temp;
}

// Return the value at head
int peek(Node** head,int* prio)
{
    if (*head != NULL) {
        *prio = (*head)->priority;
        return (*head)->data;
    } else {
        printf("Queue is empty.\n");
        // You can also return a special value or handle it differently based on your requirement.
        exit(EXIT_FAILURE);
    }
}

// Removes the element with the
// highest priority from the list
void pop(Node** head)
{
    if (*head != NULL) {
        Node* temp = *head;
        (*head) = (*head)->next;
        free(temp);
    } else {
        printf("Queue is empty. Cannot pop.\n");
        // You can also return a special value or handle it differently based on your requirement.
        exit(EXIT_FAILURE);
    }
}

// Function to push according to priority
void push(Node** head, int d, int p)
{
    if (*head == NULL) {
        // If the list is empty, create a new node and make it the head.
        *head = newNode(d, p);
    } else {
        Node* start = (*head);

        // Create new Node
        Node* temp = newNode(d, p);

        // Special Case: The head of list has lesser
        // priority than new node. So insert new
        // node before head node and change head node.
        if ((*head)->priority > p) {
            // Insert New Node before head
            temp->next = *head;
            (*head) = temp;
        } else {
            // Traverse the list and find a
            // position to insert new node
            while (start->next != NULL &&
                   start->next->priority < p) {
                start = start->next;
            }

            // Either at the ends of the list
            // or at required position
            temp->next = start->next;
            start->next = temp;
        }
    }
}

// Function to check if the list is empty
int isEmptyPrio(Node** head)
{
    return (*head) == NULL;
}
