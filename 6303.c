#include <stdio.h>
#include <stdlib.h>


typedef struct PageRef {  //struct for each referenced page
  int pageNum;  //indicates page number
  int fault;  //whether the page caused a page fault or not
  int useBit;  //used in CLOCK replacement policy only
} PageRef;


typedef struct Queue {
  PageRef* queue; //Queue of referenced pages
  int front;
  int end;
} Queue;


//functions
int OPTIMAL(Queue*, int, int, int, PageRef*, int, int, int);
int FIFO(Queue*, int, int, PageRef*, int);
int LRU(Queue*, int, int, int, PageRef*, int, int, int);
int CLOCK(Queue*, int, int, int, PageRef*, int);
PageRef initializePageRef(int, int);
Queue initializeQueue(int, int);
int isEmpty(Queue);
int isFull(Queue, int);
Queue enqueue(Queue, int, int);
Queue dequeue(Queue, int);
int searchInQueue(Queue, int, int);
void print(Queue*, int, char*, PageRef*, int, int);
void destroy(PageRef*, Queue*, Queue*, int);


int main() {
  int framesNum, pageNum, pagesCount = 0;
  char repPolicy[8];
  PageRef* pageRef;  //array of page references

  scanf("%d", &framesNum);  //number of frames in the main memory
  scanf("%s", repPolicy);  //replacement policy used

  while(1) {
    scanf("%d", &pageNum);  //scan the referenced page number
    if(pageNum == -1)  //end of page references
      break;
    if(pagesCount == 0)  //allocating size of one page (first iteration)
      pageRef = (PageRef*) malloc(sizeof(PageRef));
    else  //reallocating the size of page references array when a new page reference is encountered
      pageRef = (PageRef*) realloc(pageRef,  (pagesCount+1) * sizeof(PageRef));
    pageRef[pagesCount++] = initializePageRef(pageNum, framesNum);  //initialize the page and increment pagesCount
  }

  Queue queueStates[pagesCount];  //holds the queue state for each page reference
  Queue q = initializeQueue(framesNum * sizeof(PageRef), framesNum);  //initializing the queue that will be modified by each page reference
  int pageFaults = 0;  //number of page faults
  int flag = 0, furthest = 0, lru = pagesCount;  //will be used in OPTIMAL and LRU replacement policies

  for(int i = 0; i < pagesCount; i++) {  //looping on all page references
    queueStates[i] = initializeQueue(framesNum*sizeof(PageRef), framesNum);  // initializing the queue for pageRef[i]
    int position = searchInQueue(q, framesNum, pageRef[i].pageNum);  //search in the main memory frames (queue) if page reference exists
    if(position != -1) {  //if found
      q.queue[position].useBit = 1;  //setting useBit to 1 (used only in CLOCK replacement policy)
      for(int j = 0; j < framesNum; j++)  //setting queue for pageRef[i]
        queueStates[i].queue[j].pageNum = q.queue[j].pageNum;
      continue;  //see the next page ref.
    }
    if(!isFull(q, framesNum)) {  //if main memory frames are not full, enqueue the new page ref. and set useBit to 1
      q = enqueue(q, framesNum, pageRef[i].pageNum);
      q.queue[q.end].useBit = 1;
    }
    else {  //if the main memory frames are full, call the replacement policy used
      if(repPolicy[0] == 'O')  //OPTIMAL
        pageFaults = OPTIMAL(&q, pageFaults, pagesCount, framesNum, pageRef, i, flag, furthest);
      else if(repPolicy[0] == 'F')  //FIFO
        pageFaults = FIFO(&q, pageFaults, framesNum, pageRef, i);
      else if(repPolicy[0] == 'L')  //LRU
        pageFaults = LRU(&q, pageFaults, pagesCount, framesNum, pageRef, i, flag, lru);
      else if(repPolicy[0] == 'C')  //CLOCK
        pageFaults = CLOCK(&q,pageFaults,pagesCount,framesNum,pageRef, i);
    }
    for(int j = 0; j < framesNum; j++)  //setting queue for pageRef[i] after placing it in one of the frames
      queueStates[i].queue[j].pageNum = q.queue[j].pageNum;
  }
  print(queueStates,framesNum, repPolicy, pageRef, pagesCount, pageFaults);  //print output
  destroy(pageRef, queueStates, &q, pagesCount);  //free the allocated spaces
  return 0;
}


int OPTIMAL(Queue* q, int pageFaults, int pagesCount, int framesNum, PageRef* pageRef, int i, int flag, int furthest) {

  furthest = 0;  //indicates the future index of furthest page ref of pages existing in the main memory frames to be replaced
  for(int n = 0; n < framesNum; n++) {  //loop on page references inside the main memory frames
    flag = 0;
    for(int m = i+1; m < pagesCount; m++) {  //loop on all future page references (where i is the index of current page ref)
      if(q->queue[n].pageNum == pageRef[m].pageNum) {  //if the page residing in main memory has a future reference
        if(m > furthest) {  //compare its future reference index with the current furthest index
          furthest = m;  //update furthest
          q->front = n;  //update q->front to indicate the page that will be replaced (that has the furthest index)
        }
        flag = 1;  //there exists a future reference for this page
        break;
      }
    }
    if(flag == 0) {  //no future reference for this page
      q->front = n;  //update q->front to indicate the page that will be replaced (that has the furthest index)
      break;
    }
  }
  if(q->front == 0) //front is the first element in queue
    q->end = framesNum-1;  //then end is the last element in queue
  else
    q->end = q->front-1;

  return FIFO(q, pageFaults, framesNum, pageRef, i);  //call FIFO to replace
}


int FIFO(Queue* q, int pageFaults, int framesNum, PageRef* pageRef, int i) {
  *q = dequeue(*q, framesNum);  //remove page that is selected by q->front
  *q = enqueue(*q, framesNum, pageRef[i].pageNum);  //replace it with a new page
  q->queue[q->end].useBit = 1;  //needed for CLOCK replacement policy
  pageFaults++;
  pageRef[i].fault = 1;
  return pageFaults;
}


int LRU(Queue* q, int pageFaults, int pagesCount, int framesNum, PageRef* pageRef, int i, int flag, int lru) {

  lru = pagesCount; //reference index of lru page (since i<pagesCount, guarantees that first m=i-1 < lru)
  for(int n = 0; n < framesNum; n++) {  //loop on page references inside the main memory frames
    flag = 0;
    for(int m = i-1; m >= 0; m--) {  //loop on all previous page references (where i is the index of current page ref)
      if(q->queue[n].pageNum == pageRef[m].pageNum) {  //if the page is referenced before
        if(m < lru) {  //compare its past reference index with the current lru index
          lru = m;  //update lru
          q->front = n;  //update q->front to indicate the page that will be replaced (that has the lru index)
        }
        flag = 1;  //referenced before
        break;
      }
    }
    if(flag == 0) {  //not referenced before
      q->front = n;  //update q->front to indicate the page that will be replaced (that has the lru index)
      break;
    }
  }
  if(q->front == 0)  //front is the first element in queue
    q->end = framesNum-1;  //then end is the last element in the queue
  else
    q->end = q->front-1;

  return FIFO(q, pageFaults, framesNum, pageRef, i);  //call FIFO to replace
}


int CLOCK(Queue* q, int pageFaults, int pagesCount, int framesNum, PageRef* pageRef, int i) {

  while(q->queue[(q->end+1) % framesNum].useBit != 0) {  //looping until finding the first page with useBit = 0
    q->queue[(q->end+1) % framesNum].useBit = 0;  //setting useBit = 0
    q->end = (q->end+1) % framesNum;  //move to the next page (frame) in the queue
  }
  q->front = (q->end+1) % framesNum;  //update q->front to indicate the page that will be replaced (that has useBit = 0)
  return FIFO(q, pageFaults, framesNum, pageRef, i);  //call FIFO to replace
}


PageRef initializePageRef(int pageNum, int framesNum) {
  PageRef pageRef;
  pageRef.pageNum = pageNum;
  pageRef.fault = 0;
  pageRef.useBit = 1;
  return pageRef;
}

Queue initializeQueue(int size, int framesNum) {  //initializing the queue
  Queue q;
  q.queue = (PageRef*) malloc(size);
  for(int i = 0; i < framesNum; i++)  //initialize queue with -1 (in order not to be printed in the print function if empty)
    q.queue[i] = initializePageRef(-1, framesNum);
  q.front = -1;
  q.end = -1;
  return q;
}


int isEmpty(Queue q) {  //check if the queue is empty
  if(q.front == -1 && q.end == -1)
    return 1;
  else
    return 0;
}

int isFull(Queue q, int size) {  //check if the queue is full
  if((q.end + 1) % size == q.front)
    return 1;
  else
    return 0;
}


Queue enqueue(Queue q, int size, int value) {  //insert new value to queue
  if(isFull(q, size))
    return q;
  else if (isEmpty(q)) {
    q.front = 0;
    q.end = 0;
  }
  else
    q.end = (q.end + 1) % size;  //circular queue
  q.queue[q.end].pageNum = value;
  return q;
}


Queue dequeue(Queue q, int size) {  //remove value from queue
  if(isEmpty(q))
    return q;

  if(q.front == q.end) {  //only one element left in the queue
    q.front = -1;
    q.end = -1;
  } else
      q.front = (q.front + 1) % size;
  return q;
}


int searchInQueue(Queue q, int size, int key) {  //search if the page already exists in main memory frames (queue)
  for(int i = 0; i < size; i++) {
    if(key == q.queue[i].pageNum)
      return i;
  }
  return -1;
}


void print(Queue* queueStates, int queueSize, char* repPolicy, PageRef* pageRef, int pagesCount, int pageFaults) {  //print output
  printf("Replacement Policy = %s\n", repPolicy);
  printf("-------------------------------------\n");
  printf("Page   Content of Frames\n");
  printf("----   -----------------\n");
  for(int i = 0; i < pagesCount; i++) {
    printf("%02d", pageRef[i].pageNum);
    if(pageRef[i].fault == 1)  //check if the page reference caused page fault
      printf(" F   ");
    else
      printf("     ");
    for(int j = 0; j < queueSize; j++) {  //display queue
      if(queueStates[i].queue[j].pageNum != -1)  //if the frame is not empty
        printf("%02d ", queueStates[i].queue[j].pageNum);
    }
    printf("\n");
  }
  printf("-------------------------------------\n");
  printf("Number of page faults = %d\n", pageFaults);
}


void destroy(PageRef* pageRef, Queue* queueStates, Queue* q, int pagesCount) {  //free all allocated spaces
  free(pageRef);
  free(q->queue);
  for(int i = 0; i < pagesCount; i++)
    free(queueStates[i].queue);
}
