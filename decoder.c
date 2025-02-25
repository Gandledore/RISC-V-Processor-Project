#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* slice(char* input,int start,int end){
    char* slc = malloc(end-start+1);
    for(int i=0;i<end-start+1;i++){
        slc[i] = input[start+i];
    }
    return slc;
}

char** splitter(char* input){
    char* opcode = slice(input, 25, 31);
    char* F1 = slice(input, 19, 24);
    printf("field 1: %s\n", F1);

    printf("opcode: %s\n",opcode);
    
    if (strcmp(opcode, "1101111") == 0) { // type UJ 
        printf("type UJ\n");
        char* F2 = slice(input, 0, 21);
        printf("immediate: %s\n", F2); // TODO: have to unscramble immediate field 

        char** test = 0;
        return test; // need to break somehow?
    }
    
    char** test = 0;
    return test;
}


int main(){
    char* inp = "00000000101000000000000011101111";
    printf("input: %s\n",inp);
    char** test = splitter(inp);
    return 0;
}