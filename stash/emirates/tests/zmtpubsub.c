//  Multithreaded Hello World server

#include "zhelpers.h"
#include <pthread.h>

static void *
worker_routine (void *context) {
    //  Socket to talk to dispatcher
    void *receiver = zmq_socket (context, ZMQ_SUB);
    zsocket_set_subscribe( receiver, "" );
    zmq_connect (receiver, "inproc://workers");

    while (1) {
        char *string = s_recv (receiver);
        printf ("Received subscription: [%s]\n", string);
        free (string);
        //  Do some 'work'
        sleep (1);
    }
    zmq_close (receiver);
    return NULL;
}

int main (void)
{
    void *context = zmq_ctx_new ();

    //  Socket to talk to publisher
    void *publishers = zmq_socket (context, ZMQ_ROUTER);
    zmq_bind (publishers, "tcp://*:5555");

    //  Socket to talk to workers
    void *workers = zmq_socket (context, ZMQ_DEALER);
    zmq_bind (workers, "inproc://workers");

    //  Launch pool of worker threads
    int thread_nbr;
    for (thread_nbr = 0; thread_nbr < 5; thread_nbr++) {
        pthread_t worker;
        pthread_create (&worker, NULL, worker_routine, context);
    }
    //  Connect work threads to client threads via a queue proxy
    zmq_proxy (publishers, workers, NULL);

    //  We never get here, but clean up anyhow
    zmq_close( publishers );
    zmq_close (workers);
    zmq_ctx_destroy (context);
    return 0;
}
