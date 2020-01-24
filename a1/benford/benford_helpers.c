#include <stdio.h>

#include "benford_helpers.h"

int count_digits(int num) {
    int count = 0;
    for (int k = num; k > 0; k /= BASE) {
        count++;
    }
    return count;
}

int get_ith_from_right(int num, int i) {
    for (int j = 0; j < i; j++) {
        num /= 10; 
    } 
    return num % 10;
}

int get_ith_from_left(int num, int i) {
    int length = count_digits(num);

    printf("length: %d\n", length);    
    return get_ith_from_right(num, length - (i + 1));
}

void add_to_tally(int num, int i, int *tally) {
    int digit = get_ith_from_left(num, i);
    tally[digit]++;
    printf("digit: %d\n", digit);    
    return;
}
