#include <iostream>
#include <string>
#include <fstream>
#include "instruction.h"
#include <bitset>

int binary_to_dec(std::string num){
    int digit = 0;
    for (int i = 0; i < num.length(); i++){
        digit = digit << 1;         //bit shifts left to make room for next bit
        digit += (num[i] == '1');//adds a 1 to the end of the binary number if it is a 1, otherwise its a 0 so it adds 0
    }
    return digit;
}

class Processor{
    int pc;
    //control signals;
    int rf[32];
    int* dmem;
    int* imem;
    int imem_size;
    int dmem_size;
    public:
    Processor(int imem_size = 32, int dmem_size=32){//parse textfile, store to imem
        this->imem_size = imem_size;
        this->dmem_size = dmem_size;
        dmem = new int[imem_size];
        imem = new int[dmem_size];
        for(int i=0;i<32;i++){
            rf[i] = 0;
        }
        for(int i=0;i<dmem_size;i++){
            dmem[i]=0;
        }
        for(int i=0;i<imem_size;i++){
            imem[i]=0;
        }
        pc = 0;
    }
    bool load(std::string filepath){
        std::ifstream file(filepath);
        if (!file) {
            std::cerr << "Error opening file!" << std::endl;
            return false; // Exit with an error code
        }
        std::string line;
        for(int i=0;i<32;i++){
            if(!std::getline(file, line)){
                break;
            }
            imem[i] = binary_to_dec(line.substr(0,line.size()-1));//skips the newline character at thend
        }
        file.close(); // Close the file
        return true;
    }
    void run(){
        int inst = fetch();
        while(inst!=0){
            Instruction instruction(inst);
            std::cout << std::bitset<32>(inst) << std::endl;
            instruction.decode_instruction_fields();
            inst = fetch();
        }
    }
    int fetch(){
        int inst = imem[pc/4];
        pc+=4;
        return inst;
    }
    ~Processor(){
        delete[] dmem;
        delete[] imem;
    }
    
};