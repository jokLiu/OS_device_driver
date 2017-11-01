static struct node {
	char *message;
	char *next;
	int size;
} node;

static struct queue {
	struct node *front;
	struct node *end;
} queue;

static void init(struct queue *q);
static void destroy(struct queue *q);
static int enqueue (struct queue *q, char *message);
static char *dequeue (struct queue *q);
