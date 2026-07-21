#include <unistd.h>
#define MAX_EVENTS 64


void run_event_loop(int epoll_fd, int listen_fd){
    struct epoll_event events[MAX_EVENTS];
    while(1){
        int no_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        sleep(1);
    }
}
