
#include <stdlib.h>
#include <stdio.h>

/* struct to hold a single message in the queue */
struct node {
	char *message;
	struct node *next;
} ;

/* queue for the faster processing */
struct queue {
	struct node *front;
	struct node *end;
} ;

/* initializing the queue */
static void init(struct queue *q){
	q->front = NULL;
	q->end = NULL;
}

/* destroying the queue */
static void destroy(struct queue *q){
	struct node *head = q->front;
	struct node *temp;

	/* when the queue is destroyed during the
	   device removal, we deallocate all the messages and nodes */
	while(head){
		temp = head;
		head = head->next;
		free(temp->message);
		free(temp);
	}
}


static int enqueue (struct queue *q, char *message){
	/* we allocate the new node for storing a message */
	struct node *n = malloc(sizeof(struct node));

	/* if allocation failed, abort the current action */
	if(!n) return -1;
	n->next=NULL;
	/* assign the message to the new node */
	n->message = message;

	/* if the queue is empty we add our new node as
	   the front and end of the queue */
	if(!q->front) {
		q->front = n;
		q->end = n;
	}
		/* if queue is not empty, we add it to the end
           and make the end point to the new node */
	else {
		q->end->next = n;
		q->end = q->end->next;
	}
	//flag++;
	return 0;
}


static char *dequeue (struct queue *q){
	struct node* n;
	char* message;

	/* if the queue is empty, we return NULL
	   indicating that reading is not possible */
	if(!q->front) return NULL;

	/* we get the head of the queue to return */
	n = q->front;

	/* we retrieve the message from that node */
	message = n->message;

	/* make queue point to the next element in the queue */
	q->front = q->front->next;

	/* we free a node to release the allocated memory */
	free(n);

//	flag--;

	/* return the message */
	return message;
}

