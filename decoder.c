#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* slice(char* input,int start,int end){
    char* slc = malloc(sizeof(char)*(end-start+2));
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
    
    char** F = malloc(sizeof(char*)*6);//allocate 5 char pointers for up to 5 fields
    F[0] = malloc(sizeof(char));
    *F[0] = type;
    F[1] = slice(input, 20, 24);
    if(type=='U' || type=='J'){//U, J
        F[2] = slice(input,0,19);
    }
    else{//R,I,S,SB
        F[2] = slice(input,17,19);
        F[3] = slice(input,12,16);
        if(type=='I'){
            F[4] = slice(input,0,11);
        }
        else if(type=='R' || type =='S' || type=='B'){
            F[4] = slice(input,7,11);
            F[5] = slice(input,0,6);
        }
    }
    for(int i=0;i<6;i++){
        printf("F%d: %s\n",i,F[i]);
    }

    return F;
}

int main(){
    char* test = "00000000001100100000001010110011";
    // char* test = "00000000101001100111011010010011";
    printf("input: %s\n",test);
    char** F = splitter(test);
    for(int i=0;i<5;i++){
        free(F[i]);
    }
    return 0;
}