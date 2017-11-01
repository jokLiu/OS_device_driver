#include <linux/kernel.h>
#include <linux/module.h>
#include "queue.h"

static void init(struct queue *q){
	q->front = NULL;
	q->end = NULL;
}

static void destroy(struct queue *q){
	struct node *head = q->front;
	struct node *temp;

	while(head){
		temp = head;
		head = head->next;
		kfree(temp->message);
		kfree(temp);
	}
}


static int enqueue (struct queue *q, char *message){
	struct node *n = kmalloc(sizeof(node), GFP_KERNEL);
	struct node *temp;
	if(!n) return -ENOMEM;
	n->message = message;
	if(!q->front) {
		q->front = n;
		q->end = n;
	} else {
		q->end->next = n;
		q->end = q->end->next;
	}	
	temp = q->front;
	while(temp){
		printk(KERN_INFO "Retrieved the message: %s \n", temp->message);
		temp = temp->next;
	}

	return 0;
}


static char *dequeue (struct queue *q){
	if(!q || !q->front) return NULL;
	struct node* n = q->front;
	char* message = n->message;
	q->front = q->front->next;
	kfree(n);
	return message;
}