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
//basic errorno test
//==============================================================================
static void* _thread_errno_test(void* arg){
    pthread_exit(0);
}

int main(){
    pthread_t tid1;
        
    pthread_create(&tid1, NULL,  &_thread_errno_test, NULL);
    cout<<11111<<endl;
    pthread_join(tid1, NULL);
    sleep(1);
    cout<<22222<<endl;
    int rtn = pthread_join(tid1, NULL);
    if (rtn != ESRCH){
        cout<<"FAIL"<<endl;
        return 0;
    }else{
        cout<<"PASS"<<endl;
        return 0;
    }
    
}