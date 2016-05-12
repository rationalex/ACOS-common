#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

struct sigevent sevp;
timer_t timerid;

struct itimerspec timerspec;

struct sigaction sigact;

#define MAXVAL 5
int vals[MAXVAL];

void alrm_handler(int sig, siginfo_t *siginfo, void* ptr) {
    FILE *file;
    int val, i;
    volatile static int ones = 0;
    int average = 0;
    system("free | grep Память: | awk '{print $4}' > /tmp/tempTimer");
    file = fopen("/tmp/tempTimer", "r");
    if (file == NULL) {
        printf("fopen fail");
        exit(4);
    }
    fscanf(file, "%d", &val);
    fclose(file);
    for (i = 0; i < MAXVAL ; ++i) {
        average +=  vals[i];
    }
    vals[ones] = val;
    ones = (ones + 1) % MAXVAL;
    average += val;
    average /= MAXVAL;
    printf("Average: %d\n", average);
}

void sigint(int sig) {
    timer_delete(timerid);    
    exit(0);
}

int main() {
    int i;
    for (i = 0; i < MAXVAL; ++i) {
        vals[i] = 0;
    }
    sevp.sigev_notify = SIGEV_SIGNAL;
    sevp.sigev_signo = SIGRTMIN;

    if (timer_create(CLOCK_REALTIME, &sevp, &timerid) == -1) {
        perror("timer_create fail");
        return 1;
    }

    timerspec.it_interval.tv_sec = 0;
    timerspec.it_interval.tv_nsec = 5 * 1E8;

    timerspec.it_value = timerspec.it_interval;

    signal(SIGINT, sigint);

    sigact.sa_handler = NULL;
    sigact.sa_sigaction = alrm_handler;
    sigact.sa_flags = SA_SIGINFO; 

    if (sigaction(SIGRTMIN, &sigact, NULL) == -1) {
        perror("sigaction fail");
        return 3;
    }

    if (timer_settime(timerid, TIMER_ABSTIME, &timerspec, NULL) == - 1) {
        perror("timer_settime failed");
        return 2;
    }
    
    while(1) {
        pause();
    }

    return 0;
}
