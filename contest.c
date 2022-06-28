#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>

#define C 20 // number of competitors
#define S 3 // number of sponsors
#define CSLEEP 2
#define SSLEEP 15

int winners[S], no_winners = -1;
pthread_cond_t c;
pthread_mutex_t m;
sem_t s;

void* sponsor(void* a) {
    // get the sponsor
    int sponsor = *(int*)a;

    // some waiting time for each sponsor
    sleep(3 + rand() % SSLEEP);

    // this sponsor chooses one winner
    printf("Sponsor %d is choosing a winner...\n", sponsor);

    // choose a winner
    int winner;
    int ok = 1;
    while(ok) {
        ok = 0;
        winner = rand() % C;

        // lock to check previous winners
        pthread_mutex_lock(&m);
        for (int i = 0; i < no_winners; i++) {
            if (winners[i] == winner) ok = 1;
        }
        pthread_mutex_unlock(&m);
    }

    // lock to modify no_winners
    pthread_mutex_lock(&m);
    no_winners++;
    winners[no_winners] = winner;
    pthread_cond_broadcast(&c); // signal the change
    printf("Sponsor %d choose the winner %d\n", sponsor, winner);
    pthread_mutex_unlock(&m);
    return NULL;
}

void* competitor(void* a) {
    // get the competitor
    int ct = *(int*)a;
    int won = 0;
    int old_no_winners;

    // Check first time
    sem_wait(&s);
    printf("%d entered the website...\n", ct);
    sleep(rand() % 5 + 1);
    pthread_mutex_lock(&m);
    printf("%d is checking the winners for the first time...\n", ct);
    for (int i = 0; i <= no_winners; i++) {
        if (winners[i] == ct) {
            printf("Winner me %d!!!\n", ct);
            won = 1;
        }
    }
    old_no_winners = no_winners;
    if (no_winners == S - 1) {
        pthread_mutex_unlock(&m);
        printf("%d exited the website...\n", ct);
        sem_post(&s);
        return NULL;
    }
    pthread_mutex_unlock(&m);
    printf("%d exited the website...\n", ct);
    sem_post(&s);
    // now instead of waiting, we wait for the conditional variable
    while (won == 0 && 1) {
        pthread_mutex_lock(&m);
        while (old_no_winners == no_winners) { // we wait
            pthread_cond_wait(&c, &m);
        }
        pthread_mutex_unlock(&m);
        // if we reach this point, it means that the number of winners has changed
        sem_wait(&s);
        printf("%d entered the website again...\n", ct);
        sleep(rand() % 5 + 1);
        pthread_mutex_lock(&m);
        printf("%d is checking the winners again...\n", ct);
        for (int i = 0; i <= no_winners; i++) {
            if (winners[i] == ct) {
                printf("Winner me %d!!!\n", ct);
                won = 1;
            }
        }
        old_no_winners = no_winners;
        if (no_winners == S - 1) {
            pthread_mutex_unlock(&m);
            printf("%d exited the website...\n", ct);
            sem_post(&s);
            break;
        }
        pthread_mutex_unlock(&m);
        printf("%d exited the website...\n", ct);
        sem_post(&s);
    }
    return NULL;
}

int main (int argc, char** argv) {
    srand(time(NULL));

    pthread_t T[C + S];
    int person[C + S];

    // initialize the conditional variable and the mutex
    pthread_cond_init(&c, NULL);
    pthread_mutex_init(&m, NULL);
    sem_init(&s, 0, 5);

    // initialize the competitors
    for (int i = 0; i < C; i++) {
        person[i] = i;
        pthread_create(&T[i], NULL, competitor, (void*)&person[i]);
    }

    // initialize the sponsors
    for (int i = C; i < C + S; i++) {
        person[i] = i;
        pthread_create(&T[i], NULL, sponsor, (void*)&person[i]);
    }

    // join the competitors and sponsors
    for (int i = 0; i < C + S; i++) {
        pthread_join(T[i], NULL);
    }

    // destroy the mutex and the conditional variable
    pthread_cond_destroy(&c);
    pthread_mutex_destroy(&m);
    sem_destroy(&s);

    return 0;
}