all: sample_grader

sample_grader:autograder_main.c thread_lib 
	g++ autograder_main.c threads.o -g -o sample_grader

test: test.cpp thread_lib
	g++ test.cpp threads.o -g -o test

thread_lib:threads.cpp
	g++ -g -c threads.cpp -o threads.o

clean:
	rm threads.o
	rm test
	rm sample_grader
