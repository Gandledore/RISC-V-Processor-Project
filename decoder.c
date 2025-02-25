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
    char* opcode = slice(input,25,31);
    printf("%s",opcode);
    char** test = 0;
    return test;
}


int main(){
    char* inp = "00000000001100100000001010110011";
    char** test = splitter(inp);
    return 0;
}