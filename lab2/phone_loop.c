#include <stdio.h>

int main(){
    char phone[11];
    int num;
    int err = 0;

    scanf("%s", phone);

    while(scanf("%d", &num) == 1){
	if (num == -1) {
	    printf("%s\n", phone);
        } else if (num >= 0 && num <= 9) {
	    printf("%c\n", phone[num]);
        } else {
	    printf("ERROR\n");
	    err = 1;
        }    
    }

    return err;
}
