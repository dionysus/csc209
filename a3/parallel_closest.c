#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"

// Helper Functions-----------------------------------------------------------//
void child_fork(struct Point *p, int n, int pdmax, int *pcount, int *pipe_fd);
double getStraddleDist(struct Point *p, int n, double d);
// Wrapper Functions
int _fork();
void _pipe ( int *fd );
int _wait ( int *status );
void *_malloc(int size);
// ---------------------------------------------------------------------------//

/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {

	double d;

	// if n is small enough, use a single process
	if ( n < 4 || pdmax == 0 ) {

		d = closest_serial(p, n);

	} else {

		// Split array into left and right halves, fork a child process for each
		int mid = n/2;
		int pipe_fd[2][2];

		child_fork(p, mid, pdmax - 1, pcount, pipe_fd[0]);
		child_fork(p + mid, n - mid, pdmax - 1, pcount, pipe_fd[1]);
		
		// Wait for both child processes and update pcount
		int status;
		for (int i = 0; i < 2; i++) {
			_wait(&status);
			if (WEXITSTATUS(status)){
				*pcount += WEXITSTATUS(status);
			}
		}

		// Read the minimal distances from the child processes for each half
		double dist_left;
		double dist_right;

		if (read(pipe_fd[0][0], &dist_left, sizeof(double)) != sizeof(double)){
			perror("reading from pipe (left child)");
			exit(1);
		}
		if (read(pipe_fd[1][0], &dist_right, sizeof(double)) != sizeof(double)){
			perror("reading from pipe (right child)");
			exit(1);
		}

		// Caclulate the minimal distance between pairs of points on same half
		d = min(dist_left, dist_right);

		// Calculate the minimal distance between two points on opposite halves
		double d_straddle = getStraddleDist(p, n, d);

		// Return the minimum of d and closest distance between opposite halves
		d = min(d, d_straddle);

	}
	
	return d;
}

// HELPER FUNCTIONS ----------------------------------------------------------//

/*
 * Create a pipe for communicating with the parent and fork a child process
 * to run closest_parallel() and write distance to pipe for the parent.
 */
void child_fork(struct Point *p, int n, int pdmax, int *pcount, int *pipe_fd) {

	_pipe(pipe_fd);
	int result = _fork();

	if (result == 0) { 
		//! Child processes
		// Close reading end of pipe
		if ( close(pipe_fd[0] == -1) ) {
			perror("close reading end from child");
			exit(1);
		};

		// Recursively determine the minimum distance between pairs in the set
		double dist = closest_parallel(p, n, pdmax, pcount);
	
		if (write(pipe_fd[1], &dist, sizeof(double)) != sizeof(double)) {
			perror ("write from child to pipe");
			exit(1);
		}

		// exit with status = num of worker processes rooted at current
		exit(*pcount + 1);

	} else {
		//! Parent process
		// close writing end of pipe
		if (close(pipe_fd[1]) == -1) {
			perror("close pipe after writing");
			exit(1);
		}
	}
}

/* 
 * Given a list of Points sorted by x coordinates
 * Return the lesser of d (distance between pairs of the same half)
 * and the minimal distance between pairs of different halves.
 */
double getStraddleDist(struct Point *p, int n, double d){

	int mid = n/2;
	struct Point mid_point = p[mid];

	// Build an array strip[] that contains points close (closer than d) to the 
	// line passing through the middle point.
	struct Point *strip = _malloc(sizeof(struct Point) * n);

	int j = 0;
	for (int i = 0; i < n; i++) {
		if (abs(p[i].x - mid_point.x) < d) {
			strip[j] = p[i], j++;
		}
	}

	// find the closest points in strip
	double dist =  strip_closest(strip, j, d);
	free(strip);

	return dist;
}

// WRAPPER FUNCTIONS ---------------------------------------------------------//

/*
 * Calls fork() and check for errors
 */
int _fork() {
	int res;
	if ((res = fork()) == -1) {
		perror("fork");
		exit(1);
	} 
	return res;
}

/*
 * Calls pipe() and check for errors
 */
void _pipe ( int *fd ) {
	if(pipe(fd) == -1) {
		perror ("pipe");
		exit(1);
	} 
}

/*
 * Calls wait() and check for errors
 * Returns, if successful, the int returned by the wait() call
 */
int _wait ( int *status ) {
	int res;
	if( (res = wait(status)) == -1){
		perror("wait");
		exit(1);
	}
	return res;
}

/*
 * Calls malloc() of the given size and check for errors
 */
void *_malloc(int size) {
	void *ptr;

	if ((ptr = malloc(size)) == NULL) {
		perror("malloc");
		exit(1);
	}
	return ptr;
}
