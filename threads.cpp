//Author: Zihao Zhang
//Date: 4.21.2019

#include "threads.h"
#include <stdlib.h>
#include <stdio.h> 
#include <iostream>
#include <sys/time.h>
#include <signal.h>
#include <list>
#include <algorithm>
#include <errno.h>

using namespace std;

#define INTERVAL 50		/* time interval in milliseconds */
#define MAX 128			/* max number of threads allowed */
#define MAIN_ID 0
#define SEM_VALUE_MAX 65536		/* max semaphore value */

static sigset_t mask;	// blocking list
static int numOfThreads = 0;
static int numOfSemaphore = 0;
static pthread_t curr_thread_id = 0;
static list<TCB> thread_pool;
static list<Semaphore> semaphore_list;


void print_thread_pool(){
    printf("size = %d \n", thread_pool.size());
    list<TCB>::iterator it;
	for (it = thread_pool.begin(); it != thread_pool.end(); ++it){
    	cout << it->thread_id<<" "<<it->thread_state<<endl;
	}
	cout<<endl;
}


TCB* find_thread_by_id(pthread_t id){
	
	list<TCB>::iterator it;
	for (it = thread_pool.begin(); it != thread_pool.end(); ++it){
    	if(it->thread_id == id){
			return &*it;
		}
	}

	return NULL;	//Return NULL if not found
}

void delete_thread_by_id(pthread_t id){
	while(thread_pool.front().thread_id != id){
		thread_pool.push_back(thread_pool.front());
        thread_pool.pop_front();
	}
	thread_pool.pop_front();
	while(thread_pool.front().thread_id != curr_thread_id){
		thread_pool.push_back(thread_pool.front());
        thread_pool.pop_front();
	}
	// for (list<TCB>::iterator i = thread_pool.begin(); i != thread_pool.end(); i++){
	// 	if(i->thread_id == id){
	// 		thread_pool.erase(i);
	// 	}
	// 	break;
	// }
}


void find_next_active_thread(){
	while(thread_pool.front().thread_state != TH_ACTIVE){
		thread_pool.push_back(thread_pool.front());
        thread_pool.pop_front();
	}
}


Semaphore* find_semaphore_by_id(int id){
	list<Semaphore>::iterator it;
	for (it = semaphore_list.begin(); it != semaphore_list.end(); ++it){
    	if(it->semaphore_id == id){
			return &*it;
		}
	}
	return NULL;	//Return NULL if not found
}

void delete_semaphore_by_id(int id){
	while(semaphore_list.front().semaphore_id != id){
		semaphore_list.push_back(semaphore_list.front());
        semaphore_list.pop_front();
	}
	semaphore_list.pop_front();
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

	  	cout<<"before schdule"<<endl;
		  print_thread_pool();
		
		thread_pool.push_back(thread_pool.front());

        thread_pool.pop_front();

		find_next_active_thread();

        curr_thread_id = thread_pool.front().thread_id;

		cout<<"after schedule"<<endl;
		print_thread_pool();

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
    	perror("Error calling sigaction() !\n");
    	exit(1);
 	}
  
  	it_val.it_value.tv_sec =     INTERVAL/1000;
  	it_val.it_value.tv_usec =    (INTERVAL*1000) % 1000000;   
  	it_val.it_interval = it_val.it_value;
  
  	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
    	perror("Error calling setitimer() !\n");
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
		 numOfThreads--;
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
	cout<<"pthread_join begin"<<endl;
	print_thread_pool();
	TCB* target_thread = find_thread_by_id(thread);

	cout<<(int) thread<<endl;
	cout<<target_thread->thread_id<<endl;

	if(target_thread == NULL){
		cout<<"Error finding target thread !"<<endl;
		errno = ESRCH;
		return errno;
	}

	if(target_thread->thread_state == TH_ACTIVE || target_thread->thread_state == TH_BLOCKED){		// block and wait

		thread_pool.front().thread_state = TH_BLOCKED;
		target_thread->joinfrom_thread = curr_thread_id;
	
		thread_schedule(1);

	}

	cout<<(int) thread<<endl;
	cout<<target_thread->thread_id<<endl;

	target_thread = find_thread_by_id(thread);

	cout<<(int) thread<<endl;
	cout<<target_thread->thread_id<<endl;

	if(value_ptr != NULL){
		*value_ptr = target_thread->exit_code;
	}
	
	free(target_thread->thread_free);

	delete_thread_by_id(target_thread->thread_id);
	
	numOfThreads--;		
	cout<<"pthread_join finished"<<endl;					
	print_thread_pool();
	return 0;	//If success
}


// block alarm, disable interuption
void lock(){
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	if( sigprocmask(SIG_BLOCK, &mask, NULL) < 0 ){
		perror("Error locking !\n");
		exit(1);
	}
}


// resume interuption
void unlock(){
	if( sigprocmask(SIG_UNBLOCK, &mask, NULL) < 0 ){
		perror("Error unlocking !\n");
		exit(1);
	}
}


int sem_init(sem_t *sem, int pshared, unsigned int value){

	Semaphore new_semaphore;
	new_semaphore.semaphore_id = numOfSemaphore;
	new_semaphore.semaphore_value = value;
	new_semaphore.semaphore_max = value;
	new_semaphore.isInitialized = true;

	semaphore_list.push_back(new_semaphore);
	numOfSemaphore++;

	sem->__align = new_semaphore.semaphore_id;

	return 0;	// If success
}


int sem_destroy(sem_t *sem){

	int target_id = sem->__align;

	Semaphore* target_semaphore = find_semaphore_by_id(target_id);
	if(target_semaphore == NULL){
		perror("Error finding target semaphore !\n");
		exit(1);
	}

	if(target_semaphore->waiting_list.empty() != true){
		perror("Error destroying target semaphore : thread waiting list is not empty !\n");
		exit(1);
	}

	delete_semaphore_by_id(target_id);

	numOfSemaphore--;

	return 0;	// If success
}


int sem_wait(sem_t *sem){
	cout<<"sem_wait begin"<<endl;
	print_thread_pool();
	int target_id = sem->__align;

	Semaphore* target_semaphore = find_semaphore_by_id(target_id);
	if(target_semaphore == NULL){
		perror("Error finding target semaphore !\n");
		exit(1);
	}

	if(target_semaphore->semaphore_value > 0){

		target_semaphore->semaphore_value --;

	}else{

		thread_pool.front().thread_state = TH_BLOCKED;
		
		target_semaphore->waiting_list.push(thread_pool.front().thread_id);

		thread_schedule(1);

	}
	cout<<"sem_wait finished"<<endl;
	print_thread_pool();
	return 0;	// If success

}


int sem_post(sem_t *sem){
	cout<<"sem_post begin"<<endl;
	print_thread_pool();
	int target_id = sem->__align;

	Semaphore* target_semaphore = find_semaphore_by_id(target_id);
	if(target_semaphore == NULL){
		perror("Error finding target semaphore !\n");
		exit(1);
	}
	cout<<"sema value = "<<target_semaphore->semaphore_value<<endl;
	cout<<"waiting list = "<<target_semaphore->waiting_list.size()<<endl;
	if(target_semaphore->semaphore_value > 0){

		if(target_semaphore->semaphore_value < target_semaphore->semaphore_max){

			target_semaphore->semaphore_value++;

		}else{

			perror("Error posting semaphore: value exceed maximum !\n");
			exit(1);

		}

	}else{

		if(target_semaphore->waiting_list.empty() == true){
			
			target_semaphore->semaphore_value++;
		
		}else{
		
			pthread_t waiter_id = target_semaphore->waiting_list.front();
			TCB* waiter = find_thread_by_id(waiter_id);
			waiter->thread_state = TH_ACTIVE;

			target_semaphore->waiting_list.pop();

		}
	}
	cout<<"sem_post finished"<<endl;
	print_thread_pool();
	return 0;	// If success
		
}