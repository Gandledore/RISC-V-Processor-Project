#include <iostream>
#include <string>
#include <bitset>  // Include bitset header


using namespace std;


//slices binary number 'input', moves it all the way to the right
int slice(int input,int start,int end,bool sign_extend=false){//end is left bit index, start is right bit index
    int left = 8*sizeof(int)-end-1;
    input = input << left;
    if (sign_extend){
        return input >> (left+start);//sign extend
    }
    else{
        return static_cast<unsigned int>(input) >> (left+start);//don't sign extend
    }
}


int binary_to_dec(string num){
    int digit = 0;
    for (int i = 0; i < num.length(); i++){
        digit = digit << 1;         //bit shifts left to make room for next bit
        digit += (num[i] == '1');//adds a 1 to the end of the binary number if it is a 1, otherwise its a 0 so it adds 0
    }
    return digit;
}


enum instruction_type{//starts counting at 0
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
    // cout << "opcode: " << bitset<7>(opcode) << endl;
    
    instruction_type type = nil; // Default if instruction type isn't found

    switch(opcode) {
        case 0b1101111:
            type = UJ;
            break;
            
        case 0b0010111:
        case 0b0110111:
            type = U;
            break;
            
        case 0b0100011:
            type = S;
            break;
            
        case 0b0110011:
        case 0b0111011:
            type = R;
            break;
            
        case 0b1100011:
            type = SB;
            break;
            
        case 0b000011:
        case 0b0001111:
        case 0b0010011:
        case 0b0011011:
        case 0b1100111:
        case 0b1110011:
            type = I;
            break;
            
        default:
            type = nil;
    }
    
    int* F = new int[7]; //6-vector for fields of any instruction (some may be unused, depending on instruction) 
    // for(int i=0;i<7;i++){F[i]=0;}//ensure all fields start as 0
    
    F[0] = type; // putting instruction type into the value at F[0] address 
    F[6] = opcode; // last field is opcode

    F[1] = slice(instruction, 7, 11); // R,I: rd | S: imm[4:0] | SB: [imm[4:1|11]])

    if(type==U || type==UJ){ // U, UJ
        F[2] = slice(instruction,12,31,true); // immediate
    }
    else{ //R,I,S,SB
        F[2] = slice(instruction,12,14); // funct3
        F[3] = slice(instruction,15,19); // rs1
        if(type==I){
            F[4] = slice(instruction,20,31,true); // imm
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


void decode_R(int* F){
    cout << "Instruction Type: R" << endl;
    int funct7 = F[5];
    int funct3 = F[2];
    string operation = "";

    switch (funct7) {
        case 0b0000000:
            switch(funct3) {
                case 0b00:
                    operation = "add";
                    break;
                case 0b111:
                    operation = "and";
                    break;
                case 0b110:
                    operation = "or";
                    break;
                case 0b001:
                    operation = "sll";
                    break;
                case 0b010:
                    operation = "slt";
                    break;
                case 0b011:
                    operation = "sltu";
                    break;
                case 0b101:
                    operation = "srl";
                    break;
                case 0b100:
                    operation = "xor";
                    break;
            }
            break;
        case 0b0100000:
            switch(funct3) {
                case 0b101:
                    operation = "sra";
                    break;
                case 0b000:
                    operation = "sub";
                    break;
            }
            break;
    }

    cout << "Operation: " << operation << endl;
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


void decode_I(int* F){
    // F[1]: rd / F[4]: imm / F[2]: funct3 / F[3]: rs1
    int opcode = F[6];
    int funct3 = F[2];
    cout << "Instruction Type: I" << endl;
    string operation = "l?";

    switch (opcode) {
        case 0b0000011:
            switch(funct3){
                case 0x0: // lb
                    operation[1] = 'b';
                    break;
                case 0x1: // lh
                    operation[1] = 'h';
                    break;
                case 0x2: // lw
                    operation[1] = 'w';
                    break;
                default:
                    cout << "Undefined behavior for func3 in I type" << endl;
            }
            break;

        case 0b0010011:
            switch (funct3) {
                case 0b111:
                    operation = "andi";
                    break;
                case 0x0:
                    operation = "addi";
                    break;
                case 0b110:
                    operation = "ori";
                    break;
                case 0b010:
                    operation = "stli";
                    break;
                case 0b011:
                    operation = "stliu";
                    break;
                case 0b100:
                    operation = "xori";
                    break;
            }
            break;
    }


    if (F[6] == 0b0010011 && F[2] == 0x1 && F[4] == 0x0){ // slli
        operation = "slli";
    }

    else if (F[6] == 0b0010011 && F[2] == 0b101 && F[4] == 0x64){ // srai
        operation = "srai";
    }

    if (F[6] == 0b1100111 && F[2] == 0x0){ // jalr
        operation = "jalr";
    }

    cout << "Operation: " << operation << endl;
    cout << "Rs1: x" << F[3] << endl;
    cout << "Rd: x" << F[1] << endl;
    cout << "Immediate: " << F[4] << endl;
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
        case I:
            decode_I(F);
            break;
        default:
            cout << "Unable to decode. Instruction type not implemented." << endl;
            break;
    }
}


int main(){
    // string test = "00000000001100100000001010110011";//R
    // string test = "11111110001100100000100000100011";//S
    // string test = "00000000101001100111011010010011"; // I
    // string test = "00000000101000000000000011101111";//UJ
    // string test = "00000001111000101001001101100011";//SB

    /**
        R  | 00000000001100100000001010110011
        S  | 11111110001100100000100000100011
        I  | 00000000101001100111011010010011
        UJ | 00000000101000000000000011101111
        SB | 00000001111000101001001101100011
    */
    bool loop = true;
    string bin_inst = "";
    char ans = 'Y';
    while(loop) {
        cout << "Enter an instruction: ";
        cin >> bin_inst;
        cout << endl;
        int* F = splitter(binary_to_dec(bin_inst));
        decode_instruction_fields(F);
        delete[] F;

        cout << endl;
        // cout << "Would you like to continue (Y/N)?: ";
        // cin >> ans;
        // loop = (ans == 'Y') ? true : false;
        // cout << endl;
    }
    return 0;
}