#include <stdio.h>
#include <stdlib.h>


void print_state(int *board, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows * num_cols; i++) {
        printf("%d", board[i]);
        if (((i + 1) % num_cols) == 0) {
            printf("\n");
        }
    }
    printf("\n");
    return;
}

int check_neighbors(int *board, int i, int num_rows, int num_cols) {

    // cycle through each neighbor, incremembting count if value is 1
    int count = 0;
    for (int m = -1; m < 2; m++) {
        for (int n = -1; n < 2; n++) {
            int j = i + m * num_cols + n;
            if (j != i) {
                if (board[j] == 1) {
                    count++;
                }
            }
        }
    }

    // add new_board values based on life rules
    if (board[i] == 0) { // current is 0
        if (count == 2 || count == 3) {
            return 1;
        } else {
            return 0;
        }
    } else { // current is 1
        if (count < 2 || count > 3) {
            return 0;
        } else {
            return 1;
        }
    }
}

void update_state(int *board, int num_rows, int num_cols) {
    
    // board is unchanged if has no interior cells
    if (num_rows < 3 || num_cols < 3) {
        return;
    }

    // create new board with updated values
    int *new_board = malloc(sizeof(int) * (num_rows * num_cols));

    // scan through non-border elements
    for (int i = 0; i < num_rows * num_cols; i++) {
        // check neighbors if not edge
        if (!(i < num_cols || i > (num_cols * (num_rows - 1) - 1) || 
                i % num_cols == 0 || i % num_cols == num_cols - 1)) 
        {
            new_board[i] = check_neighbors(board, i, num_rows, num_cols);
        } else { // edge remains same
            new_board[i] = board[i];
        }
    }

    // update board
    for (int i = 0; i < num_rows * num_cols; i++) {
        board[i] = new_board[i];
    }
    free(new_board);

    return;
}

