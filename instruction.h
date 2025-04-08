#include <iostream>
#include <bitset>
#include <string>

struct decoded_data{
    int opcode;
    int read_register_1;
    int read_register_2;
    int write_register;
    int immediate;
    int alu_control;
};

struct Instruction{
    int instruction;
    int* Fields;

    enum instruction_type{//starts counting at 0
        nil,
        R,
        I,
        S,
        SB,
        U,
        UJ
    };
    
    Instruction(int instruction) {
        this->instruction = instruction;
        Fields = splitter();
    }

    decoded_data decode(){
        decoded_data data;
        data.opcode = slice(0,6,false);//opcode
        data.read_register_1 = slice(15,19,false);//read register 1
        data.read_register_2 = slice(20,24,false);//read register 2
        data.write_register = slice(7,11,false);//write register
        int immediate = 0;
        switch(Fields[0]){
            case S:{
                immediate = (Fields[5]<<5) + Fields[1];// F[5] = imm[11:5] | F[1] = imm[4:0]
                break;
            }
            case SB:{
                immediate = (Fields[5]<<5)+Fields[1];
                int bit11 = (immediate%2) << 11;                            //take bit 0 move to bit 11
                int bit12 = (immediate>>11)<<12;                            //move bit 11 and everything to the left to bit 12 and left (everything left of bit 12 is sign extended)
                int main_bits = (immediate & 0x7FE);                        //suppress bit 0 (which is bit 11), only keep bits[10:1]
                immediate = main_bits+bit11+bit12;                          //combine everything
                break;
            }
            case U:{
                immediate = slice(12,31,true);
                break;
            }
            case UJ:{
                int field = Fields[2];
                int bits19_12 = (field & 0xFF) << 12;               //only keep bits [7:0], shift to [19:12]
                int bit11 = (field & 0x00000100) << 3;              // only keep bit 8, shift to position 11
                int bits10_1 = (field & 0x0007FE00) >> 8;           //keep [18:9], shift to [10:1], now bit 0
                int bit20 = (field & 0xFFF80000)<<1;                //keep bits 19 and everything left of it, shift left to bit 20
                immediate = bit20+bits10_1+bit11+bits19_12;     //combine everything
                break;
            }
            case I:{
                immediate = Fields[4];
                break;
            }
            default:{
                immediate = 0;
                break;
            }
        }
        data.immediate = immediate;
        data.alu_control = (slice(12,14,false)<<1)+slice(30,30,false);//alu control
        return data;
    }
    
    void decode_instruction_fields(){
        switch(Fields[0]){
            case R:
                decode_R();
                break;
            case S:
                decode_S();
                break;
            case UJ:
                decode_UJ();
                break;
            case SB:
                decode_SB();
                break;
            case I:
                decode_I();
                break;
            default:
                std::cout << "Unable to decode. Instruction type not implemented." << std::endl;
                break;
        }
    }

    ~Instruction() {
        delete[] Fields;
    }
    
    private:

    //slices binary number 'input', moves it all the way to the right
    int slice(int start,int end,bool sign_extend=false){//end is left bit index, start is right bit index
        int left = 8*sizeof(int)-end-1;
        int field = instruction << left;
        if (sign_extend){
            return field >> (left+start);//sign extend
        }
        else{
            return static_cast<unsigned int>(field) >> (left+start);//don't sign extend
        }
    }

    
    int* splitter(){
        int opcode = slice(0, 6); // bits [6:0]
        
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
                std::cout << "Unknown opcode " << std::bitset<7>(opcode) << ". Can't Split." << std::endl;
                return 0;
        }
        
        int* F = new int[7]; //6-vector for fields of any instruction (some may be unused, depending on instruction) 
        memset(F, 0, 7 * sizeof(int));
        // for(int i=0;i<7;i++){F[i]=0;}//ensure all fields start as 0
        
        F[0] = type; // putting instruction type into the value at F[0] address 
        F[6] = opcode; // last field is opcode
    
        F[1] = slice(7, 11); // R,I: rd | S: imm[4:0] | SB: [imm[4:1|11]])
    
        if(type==U || type==UJ){ // U, UJ
            F[2] = slice(12,31,true); // immediate
        }
        else{ //R,I,S,SB
            F[2] = slice(12,14); // funct3
            F[3] = slice(15,19); // rs1
            if(type==I){
                F[4] = slice(20,31,true); // imm
            }
            else if(type==R || type==S || type==SB){
                F[4] = slice(20,24); // rs2
                F[5] = slice(25,31,type!=R); // R: funct7 | S,B: imm | (type!=R sign extends for S,SB imm fields)
            }
        }
    
        return F;
    }
    

    void decode_S(){
        int immediate = (Fields[5]<<5) + Fields[1];// F[5] = imm[11:5] | F[1] = imm[4:0]
        int funct3 = Fields[2];
        std::string operation = "?";
        switch(funct3){
            case 0x0:
                operation = "sb";
                break;
            case 0x1:
                operation = "sh";
                break;
            case 0x2:
                operation = "sw";
                break;
            case 0x3:
                operation = "sd";
                break;
            default:
                std::cout << "Undefined behavior for funct3 in S type" << std::endl;
        }
    
        std::cout << "Instruction Type: S" << std::endl;
        std::cout << "Operation: " << operation << std::endl;
        std::cout << "Rs1: x" << Fields[3] << std::endl;
        std::cout << "Rs2: x" << Fields[4] << std::endl;
        std::cout << "Immediate: " << immediate << std::endl;
    }
    
    
    void decode_R(){
        std::cout << "Instruction Type: R" << std::endl;
        int funct7 = Fields[5];
        int funct3 = Fields[2];
        std::string operation = "?";
    
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
                    default:
                        std::cout << "Undefined behavior for funct3 in R type" << std::endl;
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
                    default:
                        std::cout << "Undefined behavior for funct3 in R type" << std::endl;
                }
                break;
            default:
                std::cout << "Undefined behavior for funct7 in R type" << std::endl;
        }
    
        std::cout << "Operation: " << operation << std::endl;
        std::cout << "Rs1: x" << Fields[3] << std::endl;
        std::cout << "Rs2: x" << Fields[4] << std::endl;
        std::cout << "Rd: x" << Fields[1] << std::endl;
        std::cout << "Funct3: " << Fields[2] << std::endl;
        std::cout << "Funct7: " << Fields[5] << std::endl;
    }
    
    
    void decode_SB(){
        int funct3 = Fields[2];
        int immediate = (Fields[5]<<5)+Fields[1];
        int bit11 = (immediate%2) << 11;                            //take bit 0 move to bit 11
        int bit12 = (immediate>>11)<<12;                            //move bit 11 and everything to the left to bit 12 and left (everything left of bit 12 is sign extended)
        int main_bits = (immediate & 0x7FE);                        //suppress bit 0 (which is bit 11), only keep bits[10:1]
        immediate = main_bits+bit11+bit12;                          //combine everything
        
        std::string operation;
        switch(funct3){
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
                std::cout << "Undefined behavior for funct3 in SB type" << std::endl;
        }
    
        std::cout << "Instruction Type: SB" << std::endl;
        std::cout << "Operation: " << operation << std::endl;
        std::cout << "Rs1: x" <<  Fields[3] << std::endl;
        std::cout << "Rs2: x" <<  Fields[4] << std::endl;
        std::cout << "Immediate: " << immediate << std::endl;
    }
    
    
    void decode_UJ(){
        
        //unscramble immediate field
        //imm[20|10:1|11|19:12]
        int field = Fields[2];
        int bits19_12 = (field & 0xFF) << 12;               //only keep bits [7:0], shift to [19:12]
        int bit11 = (field & 0x00000100) << 3;              // only keep bit 8, shift to position 11
        int bits10_1 = (field & 0x0007FE00) >> 8;           //keep [18:9], shift to [10:1], now bit 0
        int bit20 = (field & 0xFFF80000)<<1;                //keep bits 19 and everything left of it, shift left to bit 20
        int immediate = bit20+bits10_1+bit11+bits19_12;     //combine everything
        
        std::cout << "Instruction Type: UJ" << std::endl;
        std::cout << "Operation: " << "jal" << std::endl;
        std::cout << "Rd: x" << Fields[1] << std::endl;
        std::cout << "Immediate: " << immediate << std::endl;
    }
    
    
    void decode_I(){
        // F[1]: rd / F[4]: imm / F[2]: funct3 / F[3]: rs1
        int opcode = Fields[6];
        int funct3 = Fields[2];
        int immediate = Fields[4];
        int funct7 = immediate>>5;
    
        std::string operation = "?";
    
        switch (opcode) {
            case 0b0000011:
                switch(funct3){
                    case 0x0: // lb
                        operation = "lb";
                        break;
                    case 0x1: // lh
                        operation = "lh";
                        break;
                    case 0x2: // lw
                        operation = "lw";
                        break;
                    default:
                        std::cout << "Undefined behavior for funct3 in I type. (0b0000011)" << std::endl;
                }
                break;
    
            case 0b0010011:
                switch (funct3) {
                    case 0b111:
                        operation = "andi";
                        break;
                    case 0b000:
                        operation = "addi";
                        break;
                    case 0b110:
                        operation = "ori";
                        break;
                    case 0b010:
                        operation = "slti";
                        break;
                    case 0b011:
                        operation = "sltiu";
                        break;
                    case 0b100:
                        operation = "xori";
                        break;
                    case 0b001:
                        if (funct7==0){
                            operation = "slli";
                            immediate &= 0x01F;
                        }
                        else{
                            std::cout << "Undefined behavior for funct7 in I type. (0b0010011, 0b001)" << std::endl;
                        }
                        break;
                    case 0b101:
                        if(funct7 == 0b0100000){
                            operation = "srai";
                            immediate &= 0x01F;
                        }
                        else if(funct7==0){
                            operation = "srli";
                            immediate &= 0x01F;
                        }
                        else{
                            std::cout << "Undefined behavior for funct7 in I type. (0b0010011, 0b101)" << std::endl;
                        }
                        break;
                    default:
                        std::cout << "Undefined behavior for funct3 in I type. (0b0010011)" << std::endl;
                }
                break;
            case 0b1100111:
                if (funct3==0){
                    operation = "jalr";
                }
                else{
                    std::cout << "Undefined behavior for funct3 in I type. (0b1100111)" << std::endl;
                }
                break;
            //case 0b0011011://addiw,slliw,srliw,sraiw
        }
    
        std::cout << "Instruction Type: I" << std::endl;
        std::cout << "Operation: " << operation << std::endl;
        std::cout << "Rs1: x" << Fields[3] << std::endl;
        std::cout << "Rd: x" << Fields[1] << std::endl;
        std::cout << "Immediate: " << immediate << std::endl;
    }
};

std::ostream& operator<<(std::ostream& os,Instruction& instruction){                                                 //allows a node to be printed
    os << "Instruction: " << std::bitset<32>(instruction.instruction) << "\nType:" << std::bitset<3>(instruction.Fields[0]) << "\nOpcode: " << std::bitset<7>(instruction.Fields[6]);
    for(int i=1;i<6;i++){
        os << "\nF[" << i << "]" << std::bitset<20>(instruction.Fields[i]);
    }
    return os;
}