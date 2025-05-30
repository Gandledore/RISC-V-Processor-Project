#ifndef BUFFERS_H
#define BUFFERS_H

#include <iostream>
#include <bitset>
#include <mutex>

#include "fetchBuffer.h"
#include "decodeBuffer.h"
#include "executeBuffer.h"
#include "memBuffer.h"

struct Buffer{
    protected:
    std::mutex mtx;

    public:
    virtual std::mutex& get_mutex() {return mtx;}
    virtual ~Buffer() = default;
};

struct forwarding_buffer : public Buffer{
    virtual int get_data() = 0;
};

struct fetch_buffer : public Buffer{
    fetch_buffer_data data;
};

struct decode_buffer : public forwarding_buffer{
    decode_buffer_data data;
    int get_data() override {return data.next_pc;}
};

struct execute_buffer : public forwarding_buffer{
    execute_buffer_data data;
    int get_data() override {return data.alu_result;}
};

struct mem_buffer : public forwarding_buffer{
    mem_buffer_data data;
    int get_data() override {return data.mem_read_data;}
};

#endif