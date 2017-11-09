#include "queue.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

//defines
//debug prints: set DEBUG to 0 to remove the debug prints from the output
#define DEBUG 0
#define debug_print(...) \
  do { if(DEBUG) fprintf(stderr, __VA_ARGS__); } while(0)


// --- MAIN ---
int main(int argc, char** argv){
    struct queue q;
    init(&q);
    assert(q.end == NULL);
    assert(q.front == NULL);

    char *msg;
//    = malloc(6);
//    strncpy(msg, "hello0", 6);
//    enqueue(&q, msg);
//
//    msg = malloc(6);
//    strncpy(msg, "hello2", 6);
//    enqueue(&q, msg);
////    enqueue(&q, "hello2");
////    enqueue(&q, "hello3");
//
//    assert(strncmp(dequeue(&q), "hello0",6) == 0);
//    assert(strncmp(dequeue(&q), "hello1",6) == 0);
//    assert(strncmp(dequeue(&q), "hello2",6) == 0);


    for(int i=0; i<10000; i++){
        msg = calloc(10, 1);
        strncpy(msg, "helloo", 6);
//        *(msg+5) += i;
        enqueue(&q, msg);
    }


//    char *msg2;
    for(int i=0; i<5000; i++){

//        msg2 = "helloo\0\0";

//        *(msg2+5) += i;
//        exit(1);
        msg = dequeue(&q);
        assert(strncmp(msg, "helloo", 6) == 0);
        free(msg);
    }

    destroy(&q);
    return 0;
}
