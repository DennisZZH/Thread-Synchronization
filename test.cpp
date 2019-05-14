
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>

using namespace std;

//semaphore test 2
//==============================================================================
static int a6;
static void* _thread_sem2(void* arg){
    sem_t *sem;
    int local_var, i;

    sem = (sem_t*) arg;
    cout<<66666<<endl;
    sem_wait(sem);
    cout<<77777<<endl;
    local_var = a6;
    cout<<"a6 = "<<a6<<endl;
    cout<<pthread_self()<<endl;
    for (i=0; i<5; i++) {
        local_var += pthread_self();
    }
    for (i=0; i<3; i++) {   // sleep for 3 schedules
        sleep(1);
    }
    a6 = local_var;
    cout<<88888<<endl;
    cout<<"a6 = "<<a6<<endl;
    sem_post(sem);
    pthread_exit(0);
}

int main(){
    pthread_t tid1, tid2;
    sem_t sem;

    sem_init(&sem, 0, 1);
    a6 = 10;
    cout<<11111<<endl;
    pthread_create(&tid1, NULL, &_thread_sem2, &sem);
    cout<<22222<<endl;
    pthread_create(&tid2, NULL, &_thread_sem2, &sem);
    cout<<33333<<endl;
    pthread_join(tid1, NULL);
    cout<<44444<<endl;
    pthread_join(tid2, NULL);
    cout<<55555<<endl;
    sem_destroy(&sem);
    cout<<a6<<endl;
    cout<<(10 + (tid1*5) + (tid2*5))<<endl;
    if (a6 != (10 + (tid1*5) + (tid2*5))){
        cout<< "FAIL"<<endl;
    }else{
        cout<< "PASS"<<endl;
    }

    return 0;
}