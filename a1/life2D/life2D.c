#include <stdio.h>
#include <stdlib.h>

#include "life2D_helpers.h"


int main(int argc, char **argv) {

    // life2D requires 3 input args (+1 for exec name)
    if (argc != 4) {
        fprintf(stderr, "Usage: life2D rows cols states\n");
        return 1;
    }

    // initialize variables from arguments
    int num_rows = strtol(argv[1], NULL, 10);
    int num_cols = strtol(argv[2], NULL, 10);
    int num_states = strtol(argv[3], NULL, 10);

    //make board from redirect by scanning digits directly into board array
    int *board = malloc(sizeof(int) * (num_rows * num_cols));
    int i = 0;

    while (scanf("%d", &board[i]) == 1) {
        i++;
    }

    // update board state number of times and print
    for (i = 0; i < num_states; i++) {
        if (i > 0) {
            update_state(board, num_rows, num_cols);
        }
        print_state(board, num_rows, num_cols);
    }
    free(board);
    return 0;
}
