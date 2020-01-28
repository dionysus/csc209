#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }

    // initialize tally
    int i;
    int tally[BASE] = {0};
    
    // initialize variables pos and num
    int pos = strtol(argv[1], NULL, 10);
    int num;

    // tally input from redirect
    if (argc == 2){
        while(scanf("%d", &num) == 1){
            add_to_tally(num, pos, tally);
        }
    }

    // tally input from input file
    if (argc == 3) {
        FILE *fp;
        fp = fopen(argv[2], "r");
        if (fp == NULL) {
            fprintf(stderr, "Error opening file\n");
            return 1;
        }

        while(fscanf(fp, "%d", &num) == 1) {
            add_to_tally(num, pos, tally);
        }

        if (fclose(fp) != 0) {
            fprintf(stderr, "fclose failed\n");
            return 1;
        }
    }

    // print tally
    for (i = 0; i < BASE; i++){
        printf("%ds: %d\n", i, tally[i]);
    }

    return 0;

}
