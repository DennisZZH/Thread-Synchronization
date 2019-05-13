//Author: Zihao Zhang
//Date: 4.21.2019

#include "threads.h"
#include <stdlib.h>
#include <stdio.h> 
#include <sys/time.h>
#include <signal.h>
#include <list>
#include <algorithm>

using namespace std;

#define INTERVAL 50		/* time interval in milliseconds*/
#define MAX 128			/* max number of threads allowed*/
#define MAIN_ID 0

static sigset_t mask;	// blocking list
static int numOfThreads = 0;
static pthread_t curr_thread_id = 0;
static list<TCB> thread_pool;


void print_thread_pool(){
    printf("size = %d \n", thread_pool.size());
    list<TCB>::iterator it;
	for (it = thread_pool.begin(); it != thread_pool.end(); ++it){
    	cout << it->thread_id;
	}
}


TCB* find_thread_by_id(pthread_t id){
	list<TCB>::iterator it;
	for (it = thread_pool.begin(); it != thread_pool.end(); ++it){
    	if(it->thread_id == id){
			return it;
		}
	}
	return NULL;	//Return NULL if not found
}


void find_next_active_thread(){
	while(thread_pool.front().thread_state != TH_ACTIVE){
		thread_pool.push_back(thread_pool.front());
        thread_pool.pop_front();
	}
}


// mangle function
static long int i64_ptr_mangle(long int p){
    long int ret;
    asm(" mov %1, %%rax;\n"
        " xor %%fs:0x30, %%rax;"
        " rol $0x11, %%rax;"
        " mov %%rax, %0;"
        : "=r"(ret)
        : "r"(p)
        : "%rax"
        );
        return ret;
}


void wrapper_function(){
	
	TCB executing_thread = thread_pool.front();

    void* return_value = executing_thread.thread_start_routine(thread_pool.front().thread_arg);

    pthread_exit(return_value);

}


void thread_schedule(int signo){

  if(thread_pool.size() <= 1){
      return;
  }
  
  if(setjmp(thread_pool.front().thread_buffer) == 0){
		
		thread_pool.push_back(thread_pool.front());

        thread_pool.pop_front();

		find_next_active_thread();

        curr_thread_id = thread_pool.front().thread_id;

		longjmp(thread_pool.front().thread_buffer,1);

	}
		
    return;
}


void setup_timer_and_alarm(){
	
	struct itimerval it_val;
	struct sigaction siga;

	siga.sa_handler = thread_schedule;
	siga.sa_flags =  SA_NODEFER;

 	if (sigaction(SIGALRM, &siga, NULL) == -1) {
    	perror("Error calling sigaction()\n");
    	exit(1);
 	}
  
  	it_val.it_value.tv_sec =     INTERVAL/1000;
  	it_val.it_value.tv_usec =    (INTERVAL*1000) % 1000000;   
  	it_val.it_interval = it_val.it_value;
  
  	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
    	perror("Error calling setitimer()\n");
    	exit(1);
 	}

}

//Call to initialize thread system, then add main thread to thread table
void pthread_init(){

	TCB main_thread;
	main_thread.thread_id = (pthread_t) numOfThreads;		// Main thread's id is 0
	main_thread.thread_state = TH_ACTIVE;

	main_thread.thread_start_routine = NULL;
	main_thread.thread_arg = NULL;
	main_thread.thread_free = NULL;
	main_thread.exit_code = NULL;
	main_thread.joinfrom_thread = -1;
	
	setjmp(main_thread.thread_buffer);
	
	thread_pool.push_back(main_thread);
    numOfThreads++;

	setup_timer_and_alarm();
}


int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg){
	
	// Prepare thread system
	if(numOfThreads == 0){
		pthread_init();
	}

	// Create new thread
	TCB new_thread;
	new_thread.thread_id = (pthread_t) numOfThreads;
	new_thread.thread_state = TH_ACTIVE;

	new_thread.thread_start_routine = start_routine;
	new_thread.thread_arg = arg;

	new_thread.exit_code = NULL;
	new_thread.joinfrom_thread = -1;

	setjmp(new_thread.thread_buffer);
	
	unsigned long *new_sp = (unsigned long*) malloc(32767);
	new_thread.thread_free = new_sp;

	void (*wrapper_function_ptr)() = &wrapper_function;
	
	new_thread.thread_buffer[0].__jmpbuf[6] = i64_ptr_mangle((unsigned long)(new_sp + 32767 / 8 - 2));

	new_thread.thread_buffer[0].__jmpbuf[7] = i64_ptr_mangle((unsigned long)wrapper_function_ptr);

	*thread = new_thread.thread_id;
	
	thread_pool.push_back(new_thread);
    numOfThreads++;

	return 0;	// If success

}


 void free_all_threads(){
     while(thread_pool.empty() == false){

		 if(thread_pool.front().thread_id == MAIN_ID){
			 thread_pool.pop_front();
		 }else{
			free( thread_pool.front().thread_free);
			thread_pool.pop_front();
		 }
     }
 }


void pthread_exit(void *value_ptr){
	
	if(curr_thread_id == MAIN_ID){		// main thread exit, clean up memory, terminate the process

		free_all_threads();
		exit(0);

	}else{								// regular thread exit

		thread_pool.front().thread_state = TH_DEAD;
		thread_pool.front().exit_code = value_ptr;

		if(thread_pool.front().joinfrom_thread != -1){			// If it is joint from somewhere

			TCB* join_thread = find_thread_by_id(thread_pool.front().joinfrom_thread);
			join_thread->thread_state = TH_ACTIVE;

		}

		thread_schedule(1);
        
	}
}


pthread_t pthread_self(void){
	return curr_thread_id;
}


int pthread_join(pthread_t thread, void **value_ptr){

	TCB* target_thread = find_thread_by_id(thread);
	if(target_thread == NULL){
		perror("Error finding target thread")
		exit(1);
	}
	
	if(target_thread->thread_state == TH_ACTIVE || target_thread->thread_state == TH_BLOCKED){		// block and wait

		thread_pool.front().thread_state = TH_BLOCKED;
		target_thread->joinfrom_thread = curr_thread_id;

		thread_schedule(1);

	}
	
	*value_ptr = target_thread->exit_code;
		
	free(target_thread->thread_free);
	thread_pool.erase(target_thread);							
	
	return 0;	//If success
}


// block alarm, disable interuption
void lock(){
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	if( sigprocmask(SIG_BLOCK, &mask, NULL) < 0 ){
		perror("Error locking\n");
		exit(1);
	}
}


// resume interuption
void unlock(){
	if( sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0 ){
		perror("Error unlocking\n");
		exit(1);
	}
}