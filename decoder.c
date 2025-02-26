#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* slice(char* input,int start,int end){
    char* slc = malloc(end-start+2);
    for(int i=0;i<end-start+1;i++){
        slc[i] = input[start+i];
    }
    slc[end-start+1]=0;
    return slc;
}

char** splitter(char* input){
    char* opcode = slice(input, 25, 31);
    printf("opcode: %s\n",opcode);
    
    char type = '*';
    if (strcmp(opcode, "1101111") == 0){//UJ
        type = 'J';
    }
    else if(strcmp(opcode,"0010111") == 0 || strcmp(opcode,"0110111") == 0) {//U
        type = 'U';
    }
    else if(strcmp(opcode,"0100011") == 0){//S
        type = 'S';
    }
    else if(strcmp(opcode,"0110011") == 0 || strcmp(opcode,"0111011") == 0){//R
        type = 'R';
    }
    else if(strcmp(opcode,"1100011") == 0){//SB
        type = 'B';
    }
    else if(strcmp(opcode,"000011") == 0 || strcmp(opcode,"0001111") == 0 || strcmp(opcode,"0010011") == 0 || strcmp(opcode,"0011011") == 0 || strcmp(opcode,"1100111") == 0 || strcmp(opcode,"1110011") == 0){//I
        type = 'I';
    }
    
    printf("%c\n",type);
    char* F1=0;
    char* F2=0;
    char* F3=0;
    char* F4=0;
    char* F5=0;

    F1 = slice(input, 20, 24);
    if(type=='U' || type=='J'){//U, J
        F2 = slice(input,0,19);
    }
    else{//R,I,S,SB
        F2 = slice(input,17,19);
        F3 = slice(input,12,16);
        if(type=='I'){
            F4 = slice(input,0,11);
        }
        else if(type=='R' || type =='S' || type=='B'){
            F4 = slice(input,7,11);
            F5 = slice(input,0,6);
        }
    }
    printf("F1: %s\n",F1);
    printf("F2: %s\n",F2);
    printf("F3: %s\n",F3);
    printf("F4: %s\n",F4);
    printf("F5: %s\n",F5);
    
    char** test = 0;
    return test; // need to break somehow?
}


int main(){
    char* test1 = "00000000001100100000001010110011";
    char* test2 = "00000000101001100111011010010011";
    printf("input: %s\n",test2);
    char** test = splitter(test2);
    return 0;
}