all: sample_grader

sample_grader:autograder_main.c thread_lib 
	g++ autograder_main.c threads.o -g -o sample_grader

test1: test1.cpp thread_lib
	g++ test1.cpp threads.o -g -o test1

test2: test2.cpp thread_lib
	g++ test2.cpp threads.o -g -o test2

thread_lib:threads.cpp
	g++ -g -c threads.cpp -o threads.o

clean:
	rm threads.o
	
	rm sample_grader
