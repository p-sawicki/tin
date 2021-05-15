#include <mqueue.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int main(void) {
    struct mq_attr attr = { 0 };
    attr.mq_msgsize = sizeof(int);
    attr.mq_maxmsg = 8;
    mqd_t inqueue = mq_open("/logger-test-in", O_CREAT | O_RDWR, 0777, &attr);
    mqd_t outqueue = mq_open("/logger-test-out", O_CREAT | O_RDWR, 0777, &attr);

    if(inqueue == -1) {
        perror("error creating queue");
        return 1;
    }

    sleep(5);

    int msg = 2116;
    mq_send(inqueue, (const char*)&msg, sizeof(msg), 0);
    mq_receive(outqueue, (char*)&msg, 4, NULL);
    printf("%i\n", msg);

    sleep(5);

    msg = 2149;
    mq_send(inqueue, (const char*)&msg, sizeof(msg), 0);
    mq_receive(outqueue, (char*)&msg, 4, NULL);
    printf("%i\n", msg);

    sleep(5);

    msg = -1;
    mq_send(inqueue, (const char*)&msg, sizeof(msg), 0);
    mq_receive(outqueue, (char*)&msg, 4, NULL);
    printf("%i\n", msg);

    sleep(5);

    msg = -1;
    mq_send(inqueue, (const char*)&msg, sizeof(msg), 0);
    mq_receive(outqueue, (char*)&msg, 4, NULL);
    printf("%i\n", msg);

    sleep(5);

    msg = -2;
    mq_send(inqueue, (const char*)&msg, sizeof(msg), 0);
    printf("%i\n", msg);

    close(inqueue);
    close(outqueue);
}


