#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* slice(char* input,int start,int end){
    char* slc = malloc(sizeof(char)*(end-start+2)); // +2 to include ending value and null terminator 
    for(int i=0;i<end-start+1;i++){
        slc[i] = input[start+i]; // copying the binary digits into slc 
    }
    slc[end-start+1]=0; // 0 is the null terminator 
    return slc;
}


int binary_to_dec(char* num, int len){
    int digit = 0;
    for (int i = 0; i < len; i++){
        digit += (num[i] == '1');
        digit = digit << 1;
    }

    return digit;
}


char** splitter(char* input){
    char* opcode = slice(input, 25, 31); // bits [6:0]
    // printf("opcode: %s\n",opcode);
    
    char type = '-'; // default if instruction type isn't found 
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
    else if(strcmp(opcode,"000011") == 0 || strcmp(opcode,"0001111") == 0 || strcmp(opcode,"0010011") == 0 || strcmp(opcode,"0011011") == 0 || strcmp(opcode,"1100111") == 0 || strcmp(opcode,"1110011") == 0){ //I
        type = 'I';
    }
    
    char** F = malloc(sizeof(char*)*6); //allocate 6 char pointers to store instruction type + 5 fields 

    F[0] = malloc(sizeof(char));

    *F[0] = type; // putting instruction type into the value at F[0] address 

    F[1] = slice(input, 20, 24); // R,I: rd or S: imm[4:0] or SB: [imm[4:1|11]])

    if(type=='U' || type=='J'){ // U, J
        F[2] = slice(input,0,19); // immediate
    }
    else{ //R,I,S,SB
        F[2] = slice(input,17,19); // rs1 
        F[3] = slice(input,12,16); // funct3
        if(type=='I'){
            F[4] = slice(input,0,11); // imm
        }
        else if(type=='R' || type =='S' || type=='B'){
            F[4] = slice(input,7,11); // rs2
            F[5] = slice(input,0,6); // R: funct7 or S,B: imm
        }
    }
    // for(int i=0;i<6;i++){
    //     printf("F%d: %s\n",i,F[i]);
    // }

    return F;
}


void print_R(char** F){
    printf("Instruction Type: R\n");

    if (strcmp(F[3], "000") == 0){ // add
        printf("Operation: add\n");
    }
    printf("Rs1: %c%d\n", 'x', binary_to_dec(F[2], 5));
    printf("Rs2: %c%d\n", 'x', binary_to_dec(F[4], 5));
    printf("Rd: %c%d\n", 'x', binary_to_dec(F[1], 5));
    printf("Funct3: %c\n", binary_to_dec(F[3], 3));
    printf("Funct7: %c\n", binary_to_dec(F[5], 7));
}


int main(){
    char* test = "00000000001100100000001010110011";
    // printf("input: %s\n",test);
    char** F = splitter(test);
    if (*F[0] == 'R'){
        print_R(F);
    }
    return 0;
}