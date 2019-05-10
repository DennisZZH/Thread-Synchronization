//Author: Zihao Zhang
//Date: 4.21.2019

#include <pthread.h>
#include <setjmp.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);

int pthread_join(pthread_t thread, void **value_ptr);

void pthread_exit(void *value_ptr);
    
pthread_t pthread_self(void);

void lock();

void unlock();

int sem_init(sem_t *sem, int pshared, unsigned int value);

int sem_destroy(sem_t *sem);

int sem_wait(sem_t *sem);

int sem_post(sem_t *sem);


enum State{
	TH_ACTIVE,
	TH_BLOCKED,
	TH_DEAD
};


// Thread Control Block
typedef struct {
	// Define any fields you might need inside here.
	int thread_id;
	enum State thread_state;
	jmp_buf thread_buffer;
	void *(*thread_start_routine)(void*);		// a pointer to the start_routine function
	void* thread_arg;
	unsigned long* thread_free;

} TCB;

