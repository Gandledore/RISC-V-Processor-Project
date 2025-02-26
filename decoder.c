#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// I: lw andi addi jalr lb lh ori slli slti sltiu srai srli xori
// S: sb sh sw
// SB: beq bge blt bne
// UJ: jal

char* slice(char* input,int start,int end){
    char* slc = malloc(sizeof(char)*(end-start+2)); // +2 to include ending value and null terminator 
    for(int i=0;i<end-start+1;i++){
        slc[i] = input[start+i]; // copying the binary digits into slc 
    }
    slc[end-start+1]=0; // 0 is the null terminator 
    return slc;
}


int binary_to_dec(char* num){
    int digit = 0;
    for (int i = 0; i < strlen(num); i++){
        digit = digit << 1;
        digit += (num[i] == '1');
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
    
    char** F = malloc(sizeof(char*)*7); //allocate 6 char pointers to store instruction type + 6 fields 

    F[0] = malloc(sizeof(char));
    F[6] = opcode;
    *F[0] = type; // putting instruction type into the value at F[0] address 

    F[1] = slice(input, 20, 24); // R,I: rd or S: imm[4:0] or SB: [imm[4:1|11]])

    if(type=='U' || type=='J'){ // U, J
        F[2] = slice(input,0,19); // immediate
    }
    else{ //R,I,S,SB
        F[2] = slice(input,17,19); // funct3
        F[3] = slice(input,12,16); // rs2
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

void decode_S(char** F){
    char imm[12];
    strcpy(imm,F[5]);
    strcat(imm,F[1]);
    int immediate = binary_to_dec(imm);
    if(immediate & 0x800){
        immediate |= 0xFFFFF000;
    }

    char operation[3] = "s?\0";
    switch(binary_to_dec(F[2])){
        case 0:
            operation[1] = 'b';
            break;
        case 1:
            operation[1] = 'h';
            break;
        case 2:
            operation[1] = 'w';
            break;
        case 3:
            operation[1] = 'd';
            break;
        default:
            printf("Undefined behavior for func3 in S type\n");
    }

    printf("Instruction Type: S\n");
    printf("Operation: %s\n",operation);
    printf("Rs1: x%d\n", binary_to_dec(F[3]));
    printf("Rs2: x%d\n", binary_to_dec(F[4]));
    printf("Immediate: %d\n",immediate);
}

// R: add and or sll slt sltu sra srl sub xor
void decode_R(char** F){
    printf("Instruction Type: R\n");

    if (strcmp(F[2], "000") == 0){ // add
        printf("Operation: add\n");
    }

    else if ((strcmp(F[2], "111") == 0) && (strcmp(F[5], "0000000") == 0)){ // and
        printf("Operation: and\n");
    }

    else if ((strcmp(F[2], "110") == 0) && (strcmp(F[5], "0000000") == 0)){ // or
        printf("Operation: or\n");
    }

    else if ((strcmp(F[2], "001") == 0) && (strcmp(F[5], "0000000") == 0)){ // sll
        printf("Operation: sll\n");
    }

    else if ((strcmp(F[2], "010") == 0) && (strcmp(F[5], "0000000") == 0)){ // slt
        printf("Operation: slt\n");
    }

    else if ((strcmp(F[2], "011") == 0) && (strcmp(F[5], "0000000") == 0)){ // sltu
        printf("Operation: sltu\n");
    }

    else if ((strcmp(F[2], "101") == 0) && (strcmp(F[5], "0100000") == 0)){ // sra
        printf("Operation: sra\n");
    }

    else if ((strcmp(F[2], "101") == 0) && (strcmp(F[5], "0000000") == 0)){ // srl
        printf("Operation: srl\n");
    }

    else if ((strcmp(F[2], "000") == 0) && (strcmp(F[5], "0100000") == 0)){ // sub
        printf("Operation: sub\n");
    }

    else if ((strcmp(F[2], "100") == 0) && (strcmp(F[5], "0000000") == 0)){ // xor
        printf("Operation: xorl\n");
    }

    printf("Rs1: %c%d\n", 'x',binary_to_dec(F[3]));
    printf("Rs2: %c%d\n", 'x',binary_to_dec(F[4]));
    printf("Rd: %c%d\n", 'x',binary_to_dec(F[1]));
    printf("Funct3: %d\n", binary_to_dec(F[2]));
    printf("Funct7: %d\n", binary_to_dec(F[5]));
}

void decode_instruction_fields(char** F){
    switch(*F[0]){
        case 'R':
            decode_R(F);
            break;
        case 'S':
            decode_S(F);
            break;
        default:
            printf("Type: %c not implemented",*F[0]);
            break;
    }
}


int main(){
    // char* test = "00000000001100100000001010110011";//R
    char* test = "11111110001100100000100000100011";//S
    printf("input: %s\n",test);
    char** F = splitter(test);
    decode_instruction_fields(F);
    return 0;
}