#include <iostream>
#include <string>
#include "instruction.h"

using namespace std;

int binary_to_dec(string num){
    int digit = 0;
    for (int i = 0; i < num.length(); i++){
        digit = digit << 1;         //bit shifts left to make room for next bit
        digit += (num[i] == '1');//adds a 1 to the end of the binary number if it is a 1, otherwise its a 0 so it adds 0
    }
    return digit;
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

    string bin_inst = "";
    char ans = 'Y';
    while(ans == 'Y') {
        cout << "Enter an instruction: ";
        cin >> bin_inst;
        cout << endl;
        Instruction F(binary_to_dec(bin_inst));
        F.decode();

        cout << endl;
        cout << "Would you like to continue (Y/N)?: ";
        cin >> ans;
        cout << endl;
    }
    return 0;
}