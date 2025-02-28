#include <iostream>
#include <string>
#include <bitset>  // Include bitset header

// I: lw andi addi jalr lb lh ori slli slti sltiu srai srli xori
// S: sb sh sw
// SB: beq bge blt bne
// UJ: jal

using namespace std;

int slice(int input,int start,int end,bool extend=false){//end is left bit index, start is right bit index
    int left = 8*sizeof(int)-end-1;
    input = input << left;
    if (extend){
        return input >> (left+start);
    }
    else{
        return static_cast<unsigned int>(input) >> (left+start);
    }
}

int binary_to_dec(string num){
    int digit = 0;
    for (int i = 0; i < num.length(); i++){
        digit = digit << 1;
        digit += (num[i] == '1');
    }
    return digit;
}

enum instruction_type{
    nil,
    R,
    I,
    S,
    SB,
    U,
    UJ
};

int* splitter(int instruction){
    int opcode = slice(instruction, 0, 6); // bits [6:0]
    cout << "opcode: " << bitset<7>(opcode) << endl;
    
    instruction_type type = nil; // default if instruction type isn't found
    if(opcode == 0b1101111){//UJ
        type = UJ;
    }
    else if(opcode == 0b0010111 || opcode == 0b0110111) {//U
        type = U;
    }
    else if(opcode == 0b0100011){//S
        type = S;
    }
    else if(opcode == 0b0110011 || opcode == 0b0111011){//R
        type = R;
    }
    else if(opcode == 0b1100011){//SB
        type = SB;
    }
    else if(opcode == 0b000011 || opcode == 0b0001111 || opcode == 0b0010011 || opcode == 0b0011011 || opcode == 0b1100111 || opcode == 0b1110011){ //I
        type = I;
    }
    
    int* F = new int[7]; //6-vector for fields of any instruction (some may be unused, depending on instruction) 
    for(int i=0;i<7;i++){
        F[i]=0;
    }
    F[0] = type; // putting instruction type into the value at F[0] address 
    F[6] = opcode;

    F[1] = slice(instruction, 7, 11); // R,I: rd | S: imm[4:0] | SB: [imm[4:1|11]])

    if(type==U || type==UJ){ // U, UJ
        F[2] = slice(instruction,12,31,true); // immediate
    }
    else{ //R,I,S,SB
        F[2] = slice(instruction,12,14); // funct3
        F[3] = slice(instruction,15,19); // rs1
        if(type==I){
            F[4] = slice(instruction,0,11,true); // imm
        }
        else if(type==R || type==S || type==SB){
            F[4] = slice(instruction,20,24); // rs2
            F[5] = slice(instruction,25,31,type!=R); // R: funct7 | S,B: imm | (type!=R sign extends for S,SB imm fields)
        }
    }
    // for(int i=0;i<7;i++){
    //     cout << "F" << i  << ": " << bitset<32>(F[i]) <<endl;
    // }

    return F;
}

void decode_S(int* F){
    int immediate = (F[5]<<5) + F[1];

    string operation = "s?";
    switch(F[2]){
        case 0x0:
            operation[1] = 'b';
            break;
        case 0x1:
            operation[1] = 'h';
            break;
        case 0x2:
            operation[1] = 'w';
            break;
        case 0x3:
            operation[1] = 'd';
            break;
        default:
            cout << "Undefined behavior for func3 in S type" << endl;
    }

    cout << "Instruction Type: S" << endl;
    cout << "Operation: " << operation << endl;
    cout << "Rs1: x" << F[3] << endl;
    cout << "Rs2: x" << F[4] << endl;
    cout << "Immediate: " << immediate << endl;
}

// R: add and or sll slt sltu sra srl sub xor
void decode_R(int* F){
    cout << "Instruction Type: R" << endl;

    if (F[2]==0b000){ // add
        cout << "Operation: add" << endl;
    }

    else if ((F[2]==0b111) && (F[5]==0b0000000)){ // and
        cout << "Operation: and" << endl;
    }

    else if ((F[2]==0b110) && (F[5]==0b0000000)){ // or
        cout << "Operation: or" << endl;
    }

    else if ((F[2]==0b001) && (F[5]==0b0000000)){ // sll
        cout << "Operation: sll" << endl;
    }

    else if ((F[2]==0b010) && (F[5]==0b0000000)){ // slt
        cout << "Operation: slt" << endl;
    }

    else if ((F[2]==0b011) && (F[5]==0b0000000)){ // sltu
        cout << "Operation: sltu" << endl;
    }

    else if ((F[2]==0b101) && (F[5]==0b0100000)){ // sra
        cout << "Operation: sra" << endl;
    }

    else if ((F[2]==0b101) && (F[5]==0b0000000)){ // srl
        cout << "Operation: srl" << endl;
    }

    else if ((F[2]==0b000) && (F[5]==0b0100000)){ // sub
        cout << "Operation: sub" << endl;
    }

    else if ((F[2]==0b100) && (F[5]==0b0000000)){ // xor
        cout << "Operation: xorl" << endl;
    }

    cout << "Rs1: x" << F[3] << endl;
    cout << "Rs2: x" << F[4] << endl;
    cout << "Rd: x" << F[1] << endl;
    cout << "Funct3: " << F[2] << endl;
    cout << "Funct7: " << F[5] << endl;
}

void decode_SB(int* F){
    int immediate = (F[5]<<5)+F[1];
    int bit11 = (immediate%2) << 11;  //move bit 11 to correct position
    int bit12 = (immediate>>11)<<12;//move bit 12 to correct position (everything left of bit 12 is sign extended)
    immediate = (((immediate>>1)&0x000003FF)<<1)+bit11+bit12;  //final bit is 0, then include other bits
    
    string operation;
    switch(F[2]){
        case 0x0:
            operation = "beq";
            break;
        case 0x1:
            operation = "bne";
            break;
        case 0x4:
            operation = "blt";
            break;
        case 0x5:
            operation = "bge";
            break;
        case 0x6:
            operation = "bltu";
            break;
        case 0x7:
            operation = "bgeu";
            break;
        default:
            cout << "Undefined behavior for func3 in SB type" << endl;
    }

    cout << "Instruction Type: SB" << endl;
    cout << "Operation: " << operation << endl;
    cout << "Rs1: x" <<  F[3] << endl;
    cout << "Rs2: x" <<  F[4] << endl;
    cout << "Immediate: " << immediate << endl;
}

void decode_UJ(int* F){
    //unscramble immediate field
    int field = F[2];
    int bits19_12 = (field & 0xFF) << 12; //only keep right 8 bits, shift to correct position
    int bit11 = (field & 0x00000100) << 3; // only keep bit 11 (pos 8), shift to correct position
    int bits10_1 = (field & 0x0007FE00) >> 8; //keep middle bits, shift to correct position
    int bit20 = field & 0xFFF80000; //keep 20th bit (in pos 19) and everything left of it
    int immediate = bit20+bits10_1+bit11+bits19_12;
    
    cout << "Instruction Type: UJ" << endl;
    cout << "Operation: jal" << endl;
    cout << "Rd: x" << F[1] << endl;
    cout << "Immediate: " << immediate << endl;
}

void decode_instruction_fields(int* F){
    switch(F[0]){
        case R:
            decode_R(F);
            break;
        case S:
            decode_S(F);
            break;
        case UJ:
            decode_UJ(F);
            break;
        case SB:
            decode_SB(F);
            break;
        default:
            cout << "Unable to decode. Instruction type not implemented." << endl;
            break;
    }
}


int main(){
    // string test = "00000000001100100000001010110011";//R
    // string test = "11111110001100100000100000100011";//S
    // string test = "00000000101000000000000011101111";//UJ
    string test = "00000001111000101001001101100011";//SB
    cout << "input: " << test << endl;
    int instruction = binary_to_dec(test);
    int* F = splitter(instruction);
    decode_instruction_fields(F);
    delete[] F;
    return 0;
}