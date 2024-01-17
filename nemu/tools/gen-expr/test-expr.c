// usage: ./test_expr < ./input
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

typedef uint32_t word_t;
extern word_t expr(char *e, bool *success);
extern void init_regex();

int main(){
    bool success;
    char a[3000],*exp;
    word_t correct_result,expr_result;

    init_regex();
    while (fgets(a,2999,stdin) != NULL)
    {
        a[strlen(a) - 1] = '\0';
        exp=strtok(a," ");
        exp+=strlen(a)+1;
        
        correct_result = strtoull(a,NULL,10);
        expr_result = expr(exp,&success);
        if(success != true){
            printf("make token error!\nexpr:%s",exp);
            return 0;
        }
        if(correct_result != expr_result){
            printf("expr error!\nexpr:%s\ncorrect result:%"PRIu32", expr result:%"PRIu32"\n",exp,correct_result,expr_result);
            return 0;
        }
    }
    printf("test passed!\n");
    return 0;
}
