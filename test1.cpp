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

//basic pthread join test
//==============================================================================
static void* _thread_join_test(void* arg){
    sleep(1);
    pthread_exit(0);
}

int main(){
    pthread_t tid1 = 0;
    
    pthread_create(&tid1, NULL,  &_thread_join_test, NULL);
    
    int rtn = pthread_join(tid1, NULL); //failure occurs on a timeout
    if (rtn != 0){
        cout<<"PASS"<<endl;
        return 0;
    }else{
        cout<<"FAIL"<<endl;
        return 0;
    }
    
}