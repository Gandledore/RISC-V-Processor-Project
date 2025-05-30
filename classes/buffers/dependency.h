#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include <iostream>

struct dependency{
    int buffer_index=0;//which buffer has the correct data
    int stage_needed=0;//which stage needs the data
    bool is_dependent=0;//if there is actually a dependency
};
std::ostream& operator<<(std::ostream& os, dependency& d){
    os << "(Buf: " << d.buffer_index << ", Need: " << d.stage_needed << ", Dep: " << d.is_dependent << ")";
    return os;
}
bool operator==(const dependency& d1, const dependency& d2){
    return d1.buffer_index==d2.buffer_index && d1.stage_needed==d2.stage_needed && d1.is_dependent==d2.is_dependent;
}

#endif