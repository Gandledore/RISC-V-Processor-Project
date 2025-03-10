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
    int dmem[32];
    int imem[32];
    public:
    Processor(std::string filepath){//parse textfile, store to imem
        std::ifstream file(filepath);
        if (!file) {
            std::cerr << "Error opening file!" << std::endl;
            return; // Exit with an error code
        }
    
        std::string line;
        for(int i=0;i<32;i++){
            imem[i]=0;
            dmem[i]=0;
            rf[i] = 0;
        }
        for(int i=0;i<32;i++){
            if(!std::getline(file, line)){
                break;
            }
            imem[i] = binary_to_dec(line.substr(0,line.size()-1));
        }
    
        file.close(); // Close the file
        pc = 0;
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
        int inst = imem[pc];
        pc+=1;
        return inst;
    }
    
};