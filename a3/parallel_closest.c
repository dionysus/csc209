#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"

int Fork();
void child_fork(struct Point *p, int n, int pdmax, int *pcount, int *pipe_fd);

/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {
	// p = pointer to array of points sorted according to x
	// n: number of points in array
	// pdmax: max depth of worker process tree rooted at current proc
	// pcount: (OUTPUT) number of worker processes

	printf("pdmax: %d\n", pdmax);

	//* 1) if n is small, use single process
	if ( n < 4 || pdmax == 0 ) {
		// double closest_serial(struct Point *p, int n)
		return closest_serial(p, n);
	}

	//* 2) Split the array

	int count_left = n/2;
	int count_right = n - count_left;
	struct Point *p_right = &p[count_left + 1];

	int pipe_fd[2][2];

	child_fork(p, count_left, pdmax - 1, pcount, pipe_fd[0]);
	child_fork(p_right, count_right, pdmax - 1, pcount, pipe_fd[1]);
	
	//* 4) Wait for both child processes. Increment pcount by child exit status
	
	int status;
	for (int i = 0; i < 2; i++) {
		if( wait(&status) == -1){
			perror("wait");
			exit(1);
		}
		// if (WEXITSTATUS(status)){
		// 	//TODO: figure this out?
		*pcount += WEXITSTATUS(status);
		// }
	}

	//* 5) read from child processes

	double dist_left;
	double dist_right;

	if (read(pipe_fd[0][0], &dist_left, sizeof(double)) != sizeof(double)) {
		perror("reading from pipe from left child");
		exit(1);
	}
	if (read(pipe_fd[1][0], &dist_right, sizeof(double)) != sizeof(double)) {
		perror("reading from pipe from right child");
		exit(1);
	}

	//* 6) determine distance between closet pair

	double d = min(dist_left, dist_right);

	return d;
}

//! HELPER FUNCTIONS ---------------------------------------------------------//

// call fork(), and check for errors
int Fork() {
	int result;
	if ((result = fork()) == -1) {
		perror("fork");
		exit(1);
	} 
	return result;
}

// createChild
void child_fork(struct Point *p, int n, int pdmax, int *pcount, int *pipe_fd) {
	//* 3a) create a pipe for the first child to communicate

	if(pipe(pipe_fd) == -1) {
		perror ("pipe");
		exit(1);
	};

	//* 3B) fork a child
	int result = Fork();
	if (result == 0) { // Child processes

		// Close reading end of pipe
		if ( close(pipe_fd[0] == -1) ) {
			perror("close reading end from child");
			exit(1);
		};

		//* 3Bi) call closest_parallel on left half
		// closest_parallel(struct Point *p, int n, int pdmax, int *pcount)
		double closest_dist = closest_parallel(p, n, pdmax, pcount);
	
		//* 3Bii) write to pipe
		if (write(pipe_fd[1], &closest_dist, sizeof(double)) != sizeof(double)) {
			perror ("write from child to pipe");
			exit(1);
		}

		//* 3Biii) exit with status = num of worker processes rooted at current
		//TODO: figure this part out
		exit(99);

	} else {
		// close writing end of pipe
		if (close(pipe_fd[1]) == -1) {
			perror("close pipe after writing");
			exit(1);
		}
	}
}