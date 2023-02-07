#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch-2.hpp"
#define private public //basically the devil, but oh well
#include "../src/cosproc.hpp"



uint8_t memory[65536] = { };

void MemoryWrite(uint16_t address, uint8_t value){
    //TODO: Actually put memory here
    memory[address] = value;
    //printf("Wrote %X to %X\n",value,address);
}

uint8_t MemoryRead(uint16_t address){
    //printf("READ: %X from %X\n",memory[address],address);
    return memory[address];
}

void reset(cosproc* proc){
    memset(proc->r,0,sizeof(proc->r));
	proc->pc = 0;
    proc->sp = 0xC399;
    memset(proc->st,0,sizeof(proc->st));
    memset(memory,0,sizeof(memory));
}

//Tests
TEST_CASE( "implied", "[addresssing]" ) {
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    reset(&proc);
    REQUIRE(proc.IMP() == 0); //Returns 0
}

TEST_CASE("immediate","[addressing]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    reset(&proc);
    REQUIRE(proc.IMM() == 1); //Increments program counter by 1 so 0 + 1 = 1
}

TEST_CASE("absolute","[addressing]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    reset(&proc);
    // [00 10 20] 
    memory[1] = 0x10;
    memory[2] = 0x20;
    REQUIRE(proc.ABS() == 0x1020);
}

TEST_CASE("indirect","[addressing]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    reset(&proc);
    /*
    0000:  00 00 10 ...
    0010:  00 20 ...
    */
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    REQUIRE(proc.IND() == 0x0020);
}

/* 0x00 */
TEST_CASE("nop", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    reset(&proc);
    proc.cycle();
    REQUIRE(proc.pc == 1);
}

/* 0x01 */
TEST_CASE("hcf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    reset(&proc);
    /*
    0000: 01 ...
    */
    memory[0x00] = 0x01;
    proc.cycle();
    REQUIRE(proc.pc == 0);
}

/* 0x02 */
TEST_CASE("push", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    reset(&proc);
    /*
    0000: 02 02 ...
    */
    proc.r[0] = 0xAB;
    memory[0x00] = 0x02;
    memory[0x01] = 0x02;
    proc.cycle();
    REQUIRE(memory[0xC399] == 0xAB);
    proc.r[0] = 0xBA;
    proc.cycle();
    REQUIRE(memory[0xC398] == 0xBA);
}

/* 0x03 */
TEST_CASE("pop", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    reset(&proc);
    /*
    0000: 03 03 ...
    */
    memory[0x00] = 0x03;
    memory[0x01] = 0x03;
    memory[0xC398] = 0xBB;
    memory[0xC399] = 0xCC;
    proc.sp = 0xC397;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xBB);
    proc.cycle();
    REQUIRE(proc.r[0] == 0xCC);
}

/* 0x04 */
TEST_CASE("swp", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    reset(&proc);
    /*
    0000: 04 00 01 ...
    */
    proc.r[0] = 0xAA;
    proc.r[1] = 0xBB;
    memory[0x00] = 0x04;
    memory[0x02] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xBB);
    REQUIRE(proc.r[1] == 0xAA);
}

/* 0x05-0x07 */
TEST_CASE("call", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    1110: 05 AA BB ...
    */
    reset(&proc);
    memory[0x1110] = 0x05;
    memory[0x1111] = 0xAA;
    memory[0x1112] = 0xBB;
    proc.pc = 0x1110;
    proc.cycle();
    REQUIRE(memory[0xC398] == 0x11);
    REQUIRE(memory[0xC399] == 0x10);
    REQUIRE(proc.pc == 0xAABB);
    //Abs
    /*
    1110: 06 11 20 ...
    1120: AA BB ...
    */
    reset(&proc);
    memory[0x1110] = 0x06;
    memory[0x1111] = 0x11;
    memory[0x1112] = 0x20;
    memory[0x1120] = 0xAA;
    memory[0x1121] = 0xBB;
    proc.pc = 0x1110;
    proc.cycle();
    REQUIRE(memory[0xC398] == 0x11);
    REQUIRE(memory[0xC399] == 0x10);
    REQUIRE(proc.pc == 0xAABB);
    //Ind
    /*
    1110: 07 11 20 ...
    1120: 11 22 AA BB ...
    */
    reset(&proc);
    memory[0x1110] = 0x07;
    memory[0x1111] = 0x11;
    memory[0x1112] = 0x20;
    memory[0x1120] = 0x11;
    memory[0x1121] = 0x22;
    memory[0x1122] = 0xAA;
    memory[0x1123] = 0xBB;
    proc.pc = 0x1110;
    proc.cycle();
    REQUIRE(memory[0xC398] == 0x11);
    REQUIRE(memory[0xC399] == 0x10);
    REQUIRE(proc.pc == 0xAABB);
}

/* 0x08 */
TEST_CASE("ret", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    reset(&proc);
    /*
    0000: 08 ...
    */
    memory[0x00] = 0x08;
    memory[0xC398] = 0xAA;
    memory[0xC399] = 0xCC;
    proc.sp = 0xC397;
    proc.cycle();
    REQUIRE(proc.pc == 0xAACF); //Adds 3 to jump over the original CALL
    REQUIRE(proc.sp == 0xC399);
}

/* 0x10-0x17*/
TEST_CASE("add", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    /*
    0 + 0 = 0 Z
    0000: 10 ...
    */
    reset(&proc);
    memory[0x00] = 0x10;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    0 + -1 = -1 N
    0000: 10 ...
    */
    reset(&proc);
    memory[0x00] = 0x10;
    proc.r[0] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    1 + 1 = 2
    0000: 11 00 10 ...
    0010: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x11;
    memory[0x02] = 0x10;
    memory[0x010] = 0x01;
    proc.r[0] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x02);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    0x40 + 0x40 = 0x80 ON
    0000: 12 00 10 ...
    0010: 00 12 40 ...
    */
    reset(&proc);
    memory[0x00] = 0x12;
    memory[0x02] = 0x10;
    memory[0x011] = 0x12;
    memory[0x012] = 0x40;
    proc.r[0] = 0x40;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x80);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    /*
    0xFF + 1 = 0x100 CNZ
    0000: 13 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x13;
    memory[0x01] = 0x01;
    proc.r[0] = 0xFF;
    proc.r[1] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    0 + 0 = 0 Z
    0000: 14 ...
    */
    reset(&proc);
    memory[0x00] = 0x14;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    0 + -1 = -1 N
    0000: 14 ...
    */
    reset(&proc);
    memory[0x00] = 0x14;
    proc.r[0] = 0xFF;
    proc.r[1] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.r[1] == 0xFF);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    1 + 1 = 2
    0000: 15 00 10 ...
    0010: 00 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x15;
    memory[0x02] = 0x10;
    memory[0x011] = 0x01;
    proc.r[1] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[1] == 0x02);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    0x4000 + 0x4000 = 0x8000 ON
    0000: 16 00 10 ...
    0010: 00 12 40 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x16;
    memory[0x02] = 0x10;
    memory[0x011] = 0x12;
    memory[0x012] = 0x40;
    proc.r[0] = 0x40;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x80);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    /*
    0xFFFF + 1 = 0x10000 CNZ
    0000: 17 02 ...
    */
    reset(&proc);
    memory[0x00] = 0x17;
    memory[0x01] = 0x02;
    proc.r[0] = 0xFF;
    proc.r[1] = 0xFF;
    proc.r[3] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow    
}

/* 0x18-0x1F */
TEST_CASE("sub", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    /*
    1 - 0 = 1
    0000: 18 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x18;
    proc.r[0] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x01);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    0 - 1 = -1 NCO
    0000: 19 00 10 ...
    0010: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x19;
    memory[0x02] = 0x10;
    memory[0x10] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    /*
    -1 - -1 = 0 ZO
    0000: 1A 00 10 ...
    0010: 00 12 FF ...
    */
    reset(&proc);
    memory[0x00] = 0x1A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x12] = 0xFF;
    proc.r[0] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    /*
    -1 - 2 = -3 N
    0000: 1B 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x1B;
    memory[0x01] = 0x01;
    proc.r[0] = 0xFF;
    proc.r[1] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFD);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    1 - 0 = 1
    0000: 1C 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x1C;
    proc.r[1] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x01);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    0 - 1 = -1 ZNC
    0000: 1D 00 10 ...
    0010: 00 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x1D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.r[1] == 0xFF);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    /*
    -1 - -1 = 0 ZO
    0000: 1E 00 10 ...
    0010: 00 12 FF FF ...
    */
    reset(&proc);
    memory[0x00] = 0x1E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x12] = 0xFF;
    memory[0x13] = 0xFF;
    proc.r[0] = 0xFF;
    proc.r[1] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    /*
    -1 - 2 = -3 N
    0000: 1F 02 ...
    */
    reset(&proc);
    memory[0x00] = 0x1F;
    memory[0x01] = 0x02;
    proc.r[0] = 0xFF;
    proc.r[1] = 0xFF;
    proc.r[3] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.r[1] == 0xFD);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
}

/* 0x20-0x27 */
TEST_CASE("mul", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    /*
    1 * 0 = 0 Z
    0000: 20 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x20;
    proc.r[0] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    -1 * 1 = -1 N
    0000: 21 00 10 ...
    0010: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x21;
    memory[0x02] = 0x10;
    memory[0x10] = 0x01;
    proc.r[0] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    -1 * 2 = -2 NC
    0000: 22 00 10 ...
    0010: 00 12 02 ...
    */
    reset(&proc);
    memory[0x00] = 0x22;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x12] = 0x02;
    proc.r[0] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFE);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    64 * 2 = 128 NO
    0000: 23 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x23;
    memory[0x01] = 0x01;
    proc.r[0] = 0x40;
    proc.r[1] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x80);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    /*
    -1 * -2 = 2 NCO
    0000: 24 FF FE ...
    */
    reset(&proc);
    memory[0x00] = 0x24;
    memory[0x01] = 0xFF;
    memory[0x02] = 0xFE;
    proc.r[0] = 0xFF;
    proc.r[1] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x02);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    /*
    FFFF * 0 = 0 Z
    0000: 25 00 10 ...
    0010: 00 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x25;
    memory[0x02] = 0x10;
    proc.r[0] = 0xFF;
    proc.r[1] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    2 * 4 = 8
    0000: 26 00 10 ...
    0010: 00 12 00 04 ...
    */
    reset(&proc);
    memory[0x00] = 0x26;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x13] = 0x04;
    proc.r[0] = 0x00;
    proc.r[1] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x08);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    /*
    FFFF * FFFF = 1 OCN
    0000: 27 02 ...
    */
    reset(&proc);
    memory[0x00] = 0x27;
    memory[0x01] = 0x02;
    proc.r[0] = 0xFF;
    proc.r[1] = 0xFF;
    proc.r[2] = 0xFF;
    proc.r[3] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x01);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
}

/* 0x28-0x2F */
TEST_CASE("div", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    /*
    0 / 0 = 0 E
    0000: 28 000 ...
    */
    reset(&proc);
    memory[0x00] = 0x28;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[6] == 1); //Error
    /*
    8 / 0 = 8 E
    0000: 29 00 10 ...
    0010: 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x29;
    memory[0x02] = 0x10;
    proc.r[0] = 0x08;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x08);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[6] == 1); //Error
    /*
    8 / 2 = 4
    0000: 2A 00 10 ...
    0010: 00 12 02 ...
    */
    reset(&proc);
    memory[0x00] = 0x2A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x12] = 0x02;
    proc.r[0] = 0x08;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x04);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[6] == 0); //Error
    /*
    -2 / 1 = -2 N
    0000: 2B 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x2B;
    memory[0x01] = 0x01;
    proc.r[0] = 0xFE;
    proc.r[1] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFE);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[6] == 0); //Error
    /*
    -2 / -2 = 1 O
    0000: 2C FF FE ...
    */
    reset(&proc);
    memory[0x00] = 0x2C;
    memory[0x01] = 0xFF;
    memory[0x02] = 0xFE;
    proc.r[0] = 0xFF;
    proc.r[1] = 0xFE;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x01);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    REQUIRE(proc.st[6] == 0); //Error
    /*
    5 / 2 = 2
    0000: 2D 00 10 ...
    0010: 00 02 ...
    */
    reset(&proc);
    memory[0x00] = 0x2D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x02;
    proc.r[0] = 0x00;
    proc.r[1] = 0x05;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x02);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[6] == 0); //Error
    /*
    1 / 2 = 0 Z
    0000: 2E 00 10 ...
    0010: 00 12 00 02 ...
    */
    reset(&proc);
    memory[0x00] = 0x2E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x13] = 0x02;
    proc.r[0] = 0x00;
    proc.r[1] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[6] == 0); //Error
    /*
    1 / -2 = 0 Z
    0000: 2F 02 ...
    */
    reset(&proc);
    memory[0x00] = 0x2F;
    memory[0x01] = 0x02;
    proc.r[0] = 0x00;
    proc.r[1] = 0x01;
    proc.r[2] = 0xFF;
    proc.r[3] = 0xFE;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[6] == 0); //Error
}

/* 0x30-0x3B */
TEST_CASE("mov", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm -> Abs
    /*
    0000: 30 01 00 10 ...
    0010: ...
    */
    reset(&proc);
    memory[0x00] = 0x30;
    memory[0x01] = 0x01;
    memory[0x03] = 0x10;
    proc.cycle();
    REQUIRE(memory[0x0010] == 0x01);
    //Abs -> Abs
    /*
    0000: 31 00 10 00 11 ...
    0010: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x31;
    memory[0x02] = 0x10;
    memory[0x04] = 0x11;
    memory[0x10] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x0011] == 0x01);
    //Ind -> Abs
    /*
    0000: 32 00 10 00 12 ...
    0010: 00 20 ...
    0020: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x32;
    memory[0x02] = 0x10;
    memory[0x04] = 0x12;
    memory[0x11] = 0x20;
    memory[0x20] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x0012] == 0x01);
    //Reg -> Abs
    /*
    0000: 33 00 00 10 ...
    */
    reset(&proc);
    proc.r[0] = 0x01;
    memory[0x00] = 0x33;
    memory[0x03] = 0x10;
    proc.cycle();
    REQUIRE(memory[0x0010] == 0x01);
    //Imm -> Ind
    /*
    0000: 34 01 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x34;
    memory[0x01] = 0x01;
    memory[0x03] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(memory[0x0020] == 0x01);
    //Abs -> Ind
    /*
    0000: 35 00 12 00 10 ...
    0010: 00 20 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x35;
    memory[0x02] = 0x12;
    memory[0x04] = 0x10;
    memory[0x11] = 0x20;
    memory[0x12] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x0020] == 0x01);
    //Ind -> Ind
    /*
    0000: 36 00 10 00 12 ...
    0010: 00 20 00 21 ...
    0020: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x36;
    memory[0x02] = 0x10;
    memory[0x04] = 0x12;
    memory[0x11] = 0x20;
    memory[0x13] = 0x21;
    memory[0x20] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x0021] == 0x01);
    //Reg -> Ind
    /*
    0000: 37 00 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    proc.r[0] = 0x01;
    memory[0x00] = 0x37;
    memory[0x03] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(memory[0x0020] == 0x01);
    //Imm -> Reg
    /*
    0000: 38 01 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x38;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x01);
    //Abs -> Reg
    /*
    0000: 39 00 10 00 ...
    0010: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x39;
    memory[0x02] = 0x10;
    memory[0x10] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x01);
    //Ind -> Reg
    /*
    0000: 3A 00 10 00 ...
    0010: 00 20 ...
    0020: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x3A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x20] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x01);
    //Reg -> Reg
    /*
    0000: 3B 00 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x01;
    memory[0x00] = 0x3B;
    memory[0x02] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[1] == 0x01);
}

/* 0x3C-0x3F */
TEST_CASE("shl", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    0000: 3C 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x44;
    memory[0x00] = 0x3C;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x88);
    //Abs
    /*
    0000: 3D 00 10 ...
    0010: 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x44;
    memory[0x00] = 0x3D;
    memory[0x02] = 0x10;
    memory[0x10] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x88);
    //Ind
    /*
    0000: 3E 00 10 ...
    0010: 00 12 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    memory[0x00] = 0x3E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x12] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x44);
    //Reg
    /*
    0000: 3F 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0x01;
    memory[0x00] = 0x3F;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x44);
    //Zero Flag/Carry Flag
    /*
    0000: 3F 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0xFF;
    memory[0x00] = 0x3F;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0);
    REQUIRE(proc.st[0] == 1);
    REQUIRE(proc.st[2] == 1);
}

/* 0x40-0x4B */
TEST_CASE("movx", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm -> Abs
    /*
    0000: 40 FF 01 00 10 ...
    0010: ...
    */
    reset(&proc);
    memory[0x00] = 0x40;
    memory[0x01] = 0xFF;
    memory[0x02] = 0x01;
    memory[0x04] = 0x10;
    proc.cycle();
    REQUIRE(memory[0x0010] == 0xFF);
    REQUIRE(memory[0x0011] == 0x01);
    //Abs -> Abs
    /*
    0000: 41 00 10 00 12 ...
    0010: FF 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x41;
    memory[0x02] = 0x10;
    memory[0x04] = 0x12;
    memory[0x10] = 0xFF;
    memory[0x11] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x0012] == 0xFF);
    REQUIRE(memory[0x0013] == 0x01);
    //Ind -> Abs
    /*
    0000: 42 00 10 00 12 ...
    0010: 00 20 ...
    0020: FF 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x42;
    memory[0x02] = 0x10;
    memory[0x04] = 0x12;
    memory[0x11] = 0x20;
    memory[0x20] = 0xFF;
    memory[0x21] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x0012] == 0xFF);
    REQUIRE(memory[0x0013] == 0x01);
    //Reg -> Abs
    /*
    0000: 43 00 00 10 ...
    */
    reset(&proc);
    proc.r[0] = 0xFF;
    proc.r[1] = 0x01;
    memory[0x00] = 0x43;
    memory[0x03] = 0x10;
    proc.cycle();
    REQUIRE(memory[0x0010] == 0xFF);
    REQUIRE(memory[0x0011] == 0x01);
    //Imm -> Ind
    /*
    0000: 44 FF 01 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x44;
    memory[0x01] = 0xFF;
    memory[0x02] = 0x01;
    memory[0x04] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(memory[0x0020] == 0xFF);
    REQUIRE(memory[0x0021] == 0x01);
    //Abs -> Ind
    /*
    0000: 45 00 12 00 10 ...
    0010: 00 20 FF 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x45;
    memory[0x02] = 0x12;
    memory[0x04] = 0x10;
    memory[0x11] = 0x20;
    memory[0x12] = 0xFF;
    memory[0x13] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x0020] == 0xFF);
    REQUIRE(memory[0x0021] == 0x01);
    //Ind -> Ind
    /*
    0000: 46 00 10 00 12 ...
    0010: 00 20 00 22 ...
    0020: FF 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x46;
    memory[0x02] = 0x10;
    memory[0x04] = 0x12;
    memory[0x11] = 0x20;
    memory[0x13] = 0x22;
    memory[0x20] = 0xFF;
    memory[0x21] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x0022] == 0xFF);
    REQUIRE(memory[0x0023] == 0x01);
    //Reg -> Ind
    /*
    0000: 47 00 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    proc.r[0] = 0xFF;
    proc.r[1] = 0x01;
    memory[0x00] = 0x47;
    memory[0x03] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(memory[0x0020] == 0xFF);
    REQUIRE(memory[0x0021] == 0x01);
    //Imm -> Reg
    /*
    0000: 48 FF 01 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x48;
    memory[0x01] = 0xFF;
    memory[0x02] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.r[1] == 0x01);
    //Abs -> Reg
    /*
    0000: 49 00 10 00 ...
    0010: FF 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x49;
    memory[0x02] = 0x10;
    memory[0x10] = 0xFF;
    memory[0x11] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.r[1] == 0x01);
    //Ind -> Reg
    /*
    0000: 4A 00 10 00 ...
    0010: 00 20 ...
    0020: FF 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x4A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x20] = 0xFF;
    memory[0x21] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.r[1] == 0x01);
    //Reg -> Reg
    /*
    0000: 4B 00 02 ...
    */
    reset(&proc);
    proc.r[0] = 0xFF;
    proc.r[1] = 0x01;
    memory[0x00] = 0x4B;
    memory[0x02] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[2] == 0xFF);
    REQUIRE(proc.r[3] == 0x01);
}

/* 0x4C-0x4F */
TEST_CASE("shlx", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    0000: 4C 00 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x4C;
    memory[0x02] = 0x01;
    proc.r[0] = 0x44;
    proc.r[1] = 0x22;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x88);
    REQUIRE(proc.r[1] == 0x44);
    //Abs
    /*
    0000: 4D 00 10 ...
    0010: 00 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x44;
    proc.r[1] = 0x22;
    memory[0x00] = 0x4D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x88);
    REQUIRE(proc.r[1] == 0x44);
    //Ind
    /*
    0000: 4E 00 10 ...
    0010: 00 12 00 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0x44;
    memory[0x00] = 0x4E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x13] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x44);
    REQUIRE(proc.r[1] == 0x88);
    //Reg
    /*
    0000: 4F 02 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0x44;
    proc.r[3] = 0x01;
    memory[0x00] = 0x4F;
    memory[0x01] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x44);
    REQUIRE(proc.r[1] == 0x88);
    //Zero Flag/Carry Flag
    /*
    0000: 4F 02 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0x44;
    proc.r[3] = 0xFF;
    memory[0x00] = 0x4F;
    memory[0x01] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 1);
    REQUIRE(proc.st[2] == 1);
    //Extra Case
    /*
    0000: 4F 02 ...
    */
    reset(&proc);
    proc.r[1] = 0x80;
    proc.r[3] = 0x01;
    memory[0x00] = 0x4F;
    memory[0x01] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x01);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 0);
    REQUIRE(proc.st[2] == 0);
}

/* 0x50-0x53 */
TEST_CASE("and", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    0000: 50 FF ...
    */
    reset(&proc);
    proc.r[0] = 0x0F;
    memory[0x00] = 0x50;
    memory[0x01] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x0F);
    REQUIRE(proc.st[0] == 0);
    //Abs
    /*
    0000: 51 00 03 FF ...
    */
    reset(&proc);
    proc.r[0] = 0x0F;
    memory[0x00] = 0x51;
    memory[0x02] = 0x03;
    memory[0x03] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x0F);
    REQUIRE(proc.st[0] == 0);
    //Ind
    /*
    0000: 52 00 03 00 05 FF ...
    */
    reset(&proc);
    proc.r[0] = 0xF0;
    memory[0x00] = 0x52;
    memory[0x02] = 0x03;
    memory[0x04] = 0x05;
    memory[0x05] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xF0);
    REQUIRE(proc.st[0] == 0);
    //Reg
    /*
    0000: 53 01 ...
    */
    reset(&proc);
    proc.r[0] = 0xF0;
    proc.r[1] = 0xFF;
    memory[0x00] = 0x53;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xF0);
    REQUIRE(proc.st[0] == 0);
    //Zero Flag
    /*
    0000: 53 01 ...
    */
    reset(&proc);
    proc.r[0] = 0xF0;
    proc.r[1] = 0x0F;
    memory[0x00] = 0x53;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 1);
}

/* 0x54-0x57 */
TEST_CASE("or", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    /*
    0000: 54 F0 ...
    */
    reset(&proc);
    proc.r[0] = 0x0F;
    memory[0x00] = 0x54;
    memory[0x01] = 0xF0;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0);
    //Abs
    /*
    0000: 55 00 03 F0 ...
    */
    reset(&proc);
    proc.r[0] = 0x0F;
    memory[0x00] = 0x55;
    memory[0x02] = 0x03;
    memory[0x03] = 0xF0;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0);
    //Ind
    /*
    0000: 56 00 03 00 05 0F ...
    */
    reset(&proc);
    proc.r[0] = 0xF0;
    memory[0x00] = 0x56;
    memory[0x02] = 0x03;
    memory[0x04] = 0x05;
    memory[0x05] = 0x0F;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0);
    //Reg
    /*
    0000: 57 01 ...
    */
    reset(&proc);
    proc.r[0] = 0xF0;
    proc.r[1] = 0xFF;
    memory[0x00] = 0x57;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0);
    //Zero Flag
    /*
    0000: 57 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x57;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 1);
}

/* 0x58-0x5B */
TEST_CASE("xor", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    0000: 58 F0 ...
    */
    reset(&proc);
    proc.r[0] = 0x0F;
    memory[0x00] = 0x58;
    memory[0x01] = 0xF0;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0);
    //Abs
    /*
    0000: 59 00 03 F0 ...
    */
    reset(&proc);
    proc.r[0] = 0xFF;
    memory[0x00] = 0x59;
    memory[0x02] = 0x03;
    memory[0x03] = 0xF0;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x0F);
    REQUIRE(proc.st[0] == 0);
    //Ind
    /*
    0000: 5A 00 03 00 05 0F ...
    */
    reset(&proc);
    proc.r[0] = 0xF0;
    memory[0x00] = 0x5A;
    memory[0x02] = 0x03;
    memory[0x04] = 0x05;
    memory[0x05] = 0x0F;
    proc.cycle();
    REQUIRE(proc.r[0] == 0xFF);
    REQUIRE(proc.st[0] == 0);
    //Reg
    /*
    0000: 5B 01 ...
    */
    reset(&proc);
    proc.r[0] = 0xF0;
    proc.r[1] = 0xFF;
    memory[0x00] = 0x5B;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x0F);
    REQUIRE(proc.st[0] == 0);
    //Zero Flag
    /*
    0000: 5B 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x5B;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.st[0] == 1);
}

/* 0x05C-0x5F */
TEST_CASE("shr", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    0000: 5C 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x88;
    memory[0x00] = 0x5C;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x44);
    //Abs
    /*
    0000: 5D 00 10 ...
    0010: 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x44;
    memory[0x00] = 0x5D;
    memory[0x02] = 0x10;
    memory[0x10] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x22);
    //Ind
    /*
    0000: 5E 00 10 ...
    0010: 00 12 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    memory[0x00] = 0x5E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x12] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x11);
    //Reg
    /*
    0000: 5F 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0x01;
    memory[0x00] = 0x5F;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x11);
    //Zero Flag
    /*
    0000: 5F 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0xFF;
    memory[0x00] = 0x5F;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0);
    REQUIRE(proc.st[0] == 1);
}

/* 0x60-0x63 */
TEST_CASE("cmp", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    0000: 60 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x60;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[4] == 1); //Less
    //Abs
    /*
    0000: 61 00 10 ...
    0010: 00 ...
    */
    reset(&proc);
    proc.r[0] = 0x01;
    memory[0x00] = 0x61;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    REQUIRE(proc.st[4] == 0); //Less
    //Ind
    /*
    0000: 62 00 10 ...
    0010: 00 12 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x62;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    proc.cycle();
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[4] == 0); //Less
    //Reg
    /*
    0000: 63 01 ...
    */
    reset(&proc);
    proc.r[1] = 0xFF;
    memory[0x00] = 0x63;
    memory[0x01] = 0x01;
    proc.cycle();
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    REQUIRE(proc.st[4] == 0); //Less
}

/* 0x64-0x67 */
TEST_CASE("cmpx", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    0000: 64 00 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x64;
    memory[0x02] = 0x01;
    proc.cycle();
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[4] == 1); //Less
    //Abs
    /*
    0000: 65 00 10 ...
    0010: 00 00 ...
    */
    reset(&proc);
    proc.r[0] = 0x01;
    memory[0x00] = 0x65;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    REQUIRE(proc.st[4] == 0); //Less
    //Ind
    /*
    0000: 66 00 10 ...
    0010: 00 12 00 00 ...
    */
    reset(&proc);
    memory[0x00] = 0x66;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    proc.cycle();
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    REQUIRE(proc.st[4] == 0); //Less
    //Reg
    /*
    0000: 67 02 ...
    */
    reset(&proc);
    proc.r[2] = 0xFF;
    proc.r[3] = 0xFF;
    memory[0x00] = 0x67;
    memory[0x01] = 0x02;
    proc.cycle();
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    REQUIRE(proc.st[4] == 0); //Less
}

/* 0x6C-0x6F */
TEST_CASE("shrx", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    /*
    0000: 6C 00 01 ...
    */
    reset(&proc);
    memory[0x00] = 0x6C;
    memory[0x02] = 0x01;
    proc.r[0] = 0x44;
    proc.r[1] = 0x22;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x22);
    REQUIRE(proc.r[1] == 0x11);
    //Abs
    /*
    0000: 6D 00 10 ...
    0010: 00 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x44;
    proc.r[1] = 0x22;
    memory[0x00] = 0x6D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x22);
    REQUIRE(proc.r[1] == 0x11);
    //Ind
    /*
    0000: 6E 00 10 ...
    0010: 00 12 00 01 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0x44;
    memory[0x00] = 0x6E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x13] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x11);
    REQUIRE(proc.r[1] == 0x22);
    //Reg
    /*
    0000: 6F 02 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0x44;
    proc.r[3] = 0x01;
    memory[0x00] = 0x6F;
    memory[0x01] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x11);
    REQUIRE(proc.r[1] == 0x22);
    //Zero Flag
    /*
    0000: 6F 02 ...
    */
    reset(&proc);
    proc.r[0] = 0x22;
    proc.r[1] = 0x44;
    proc.r[3] = 0xFF;
    memory[0x00] = 0x6F;
    memory[0x01] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 1);
    //Extra Case
    /*
    0000: 6F 02 ...
    */
    reset(&proc);
    proc.r[0] = 0x01;
    proc.r[1] = 0x00;
    proc.r[3] = 0x01;
    memory[0x00] = 0x6F;
    memory[0x01] = 0x02;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x00);
    REQUIRE(proc.r[1] == 0x80);
    REQUIRE(proc.st[0] == 0);
}

/* 0x70-0x73 */
TEST_CASE("jmp", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    // [70 00 10]
    reset(&proc);
    memory[0x00] = 0x70;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Abs
    /*
    0000: 71 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x71;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Ind
    /*
    0000: 72 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x72;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Reg
    // [73 00]
    reset(&proc);
    memory[0x00] = 0x73;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
}

/* 0x74-0x77 */
TEST_CASE("jzs", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not zero
    // [74 00 10]
    reset(&proc);
    memory[0x00] = 0x74;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Zero
    // [74 00 10]
    reset(&proc);
    memory[0x00] = 0x74;
    memory[0x02] = 0x10;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Abs
    //Not zero
    /*
    0000: 75 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x75;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Zero
    /*
    0000: 75 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x75;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Ind
    //Not zero
    /*
    0000: 76 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x76;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Zero
    /*
    0000: 76 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x76;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Reg
    //Not zero
    // [77 00]
    reset(&proc);
    memory[0x00] = 0x77;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
    //Zero
    // [77 00]
    reset(&proc);
    memory[0x00] = 0x77;
    proc.r[1] = 0x10;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
}

/* 0x78-0x7B */
TEST_CASE("jnz", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not zero
    // [78 00 10]
    reset(&proc);
    memory[0x00] = 0x78;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Zero
    // [78 00 10]
    reset(&proc);
    memory[0x00] = 0x78;
    memory[0x02] = 0x10;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Abs
    //Not zero
    /*
    0000: 79 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x79;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Zero
    /*
    0000: 79 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x79;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Not zero
    /*
    0000: 7A 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x7A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Ind
    //Zero
    /*
    0000: 7A 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x7A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Reg
    //Not zero
    // [7B 00]
    reset(&proc);
    memory[0x00] = 0x7B;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Zero
    // [7B 00]
    reset(&proc);
    memory[0x00] = 0x7B;
    proc.r[1] = 0x10;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
}

/* 0x7C-0x7F */
TEST_CASE("jcs", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not carry
    // [7C 00 10]
    reset(&proc);
    memory[0x00] = 0x7C;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Carry
    // [7C 00 10]
    reset(&proc);
    memory[0x00] = 0x7C;
    memory[0x02] = 0x10;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Abs
    //Not carry
    /*
    0000: 7D 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x7D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Carry
    /*
    0000: 7D 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x7D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Ind
    //Not carry
    /*
    0000: 7E 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x7E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Carry
    /*
    0000: 7E 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x7E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Reg
    //Not carry
    // [7F 00]
    reset(&proc);
    memory[0x00] = 0x7F;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
    //Carry
    // [7F 00]
    reset(&proc);
    memory[0x00] = 0x7F;
    proc.r[1] = 0x10;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
}

/* 0x80-0x83 */
TEST_CASE("jnc", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not carry
    // [80 00 10]
    reset(&proc);
    memory[0x00] = 0x80;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Carry
    // [80 00 10]
    reset(&proc);
    memory[0x00] = 0x80;
    memory[0x02] = 0x10;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Abs
    //Not carry
    /*
    0000: 81 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x81;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Carry
    /*
    0000: 81 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x81;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Not carry
    /*
    0000: 82 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x82;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Ind
    //Carry
    /*
    0000: 82 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x82;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Reg
    //Not carry
    // [83 00]
    reset(&proc);
    memory[0x00] = 0x83;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Carry
    // [83 00]
    reset(&proc);
    memory[0x00] = 0x83;
    proc.r[1] = 0x10;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
}

/* 0x84-0x87 */
TEST_CASE("jos", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not overflow
    // [84 00 10]
    reset(&proc);
    memory[0x00] = 0x84;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Overflow
    // [84 00 10]
    reset(&proc);
    memory[0x00] = 0x84;
    memory[0x02] = 0x10;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Abs
    //Not overflow
    /*
    0000: 85 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x85;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Overflow
    /*
    0000: 85 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x85;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Ind
    //Not overflow
    /*
    0000: 86 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x86;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Overflow
    /*
    0000: 86 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x86;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Reg
    //Not overflow
    // [87 00]
    reset(&proc);
    memory[0x00] = 0x87;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
    //Overflow
    // [87 00]
    reset(&proc);
    memory[0x00] = 0x87;
    proc.r[1] = 0x10;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
}

/* 0x88-0x8B */
TEST_CASE("jno", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not overflow
    // [88 00 10]
    reset(&proc);
    memory[0x00] = 0x88;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Overflow
    // [88 00 10]
    reset(&proc);
    memory[0x00] = 0x88;
    memory[0x02] = 0x10;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Abs
    //Not overflow
    /*
    0000: 89 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x89;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Overflow
    /*
    0000: 89 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x89;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Not overflow
    /*
    0000: 8A 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x8A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Ind
    //Overflow
    /*
    0000: 8A 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x8A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Reg
    //Not overflow
    // [8B 00]
    reset(&proc);
    memory[0x00] = 0x8B;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Overflow
    // [8B 00]
    reset(&proc);
    memory[0x00] = 0x8B;
    proc.r[1] = 0x10;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
}

/* 0x8C-0x8F */
TEST_CASE("jns", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not negative
    // [8C 00 10]
    reset(&proc);
    memory[0x00] = 0x8C;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Negative
    // [8C 00 10]
    reset(&proc);
    memory[0x00] = 0x8C;
    memory[0x02] = 0x10;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Abs
    //Not negative
    /*
    0000: 8D 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x8D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Negative
    /*
    0000: 8D 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x8D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Ind
    //Not negative
    /*
    0000: 8E 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x8E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Negative
    /*
    0000: 8E 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x8E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Reg
    //Not negative
    // [8F 00]
    reset(&proc);
    memory[0x00] = 0x8F;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
    //Negative
    // [8F 00]
    reset(&proc);
    memory[0x00] = 0x8F;
    proc.r[1] = 0x10;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
}

/* 0x90-0x93 */
TEST_CASE("jnn", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not negative
    // [90 00 10]
    reset(&proc);
    memory[0x00] = 0x90;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Negative
    // [90 00 10]
    reset(&proc);
    memory[0x00] = 0x90;
    memory[0x02] = 0x10;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Abs
    //Not negative
    /*
    0000: 91 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x91;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Negative
    /*
    0000: 91 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x91;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Not negative
    /*
    0000: 92 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x92;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Ind
    //Negative
    /*
    0000: 92 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x92;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Reg
    //Not negative
    // [93 00]
    reset(&proc);
    memory[0x00] = 0x93;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Negative
    // [93 00]
    reset(&proc);
    memory[0x00] = 0x93;
    proc.r[1] = 0x10;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
}

/* 0x94-0x97 */
TEST_CASE("jls", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not less
    // [94 00 10]
    reset(&proc);
    memory[0x00] = 0x94;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Less
    // [94 00 10]
    reset(&proc);
    memory[0x00] = 0x94;
    memory[0x02] = 0x10;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Abs
    //Not less
    /*
    0000: 95 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x95;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Less
    /*
    0000: 95 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x95;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Ind
    //Not less
    /*
    0000: 96 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x96;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Less
    /*
    0000: 96 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x96;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Reg
    //Not less
    // [97 00]
    reset(&proc);
    memory[0x00] = 0x97;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
    //Less
    // [97 00]
    reset(&proc);
    memory[0x00] = 0x97;
    proc.r[1] = 0x10;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
}

/* 0x98-0x9B */
TEST_CASE("jnl", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite); 
    //Imm
    //Not less
    // [98 00 10]
    reset(&proc);
    memory[0x00] = 0x98;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Less
    // [98 00 10]
    reset(&proc);
    memory[0x00] = 0x98;
    memory[0x02] = 0x10;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Abs
    //Not less
    /*
    0000: 99 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x99;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Less
    /*
    0000: 99 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x99;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Not less
    /*
    0000: 9A 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x9A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Ind
    //Less
    /*
    0000: 9A 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x9A;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Reg
    //Not less
    // [9B 00]
    reset(&proc);
    memory[0x00] = 0x9B;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Less
    // [9B 00]
    reset(&proc);
    memory[0x00] = 0x9B;
    proc.r[1] = 0x10;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
}

/* 0x9C-0x9F */
TEST_CASE("jes", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //Imm
    //Not error
    // [9C 00 10]
    reset(&proc);
    memory[0x00] = 0x9C;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Error
    // [9C 00 10]
    reset(&proc);
    memory[0x00] = 0x9C;
    memory[0x02] = 0x10;
    proc.st[6] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
    //Abs
    //Not error
    /*
    0000: 9D 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x9D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Error
    /*
    0000: 9D 00 10 ...
    0010: 00 20 ...
    */
    reset(&proc);
    memory[0x00] = 0x9D;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    proc.st[6] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0020);
    //Ind
    //Not error
    /*
    0000: 9E 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x9E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.cycle();
    REQUIRE(proc.pc == 0x0003);
    //Ind
    //Error
    /*
    0000: 9E 00 10 ...
    0010: 00 20 ...
    0020: 00 30 ...
    */
    reset(&proc);
    memory[0x00] = 0x9E;
    memory[0x02] = 0x10;
    memory[0x11] = 0x20;
    memory[0x21] = 0x30;
    proc.st[6] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0030);
    //Reg
    //Not error
    // [9F 00]
    reset(&proc);
    memory[0x00] = 0x9F;
    proc.r[1] = 0x10;
    proc.cycle();
    REQUIRE(proc.pc == 0x0002);
    //Less
    // [9F 00]
    reset(&proc);
    memory[0x00] = 0x9F;
    proc.r[1] = 0x10;
    proc.st[6] = 1;
    proc.cycle();
    REQUIRE(proc.pc == 0x0010);
}

/* 0xA0 */
TEST_CASE("csf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flags set
    /*
    0000: A0 ...
    */
    reset(&proc);
    memory[0x00] = 0xA0;
    proc.cycle();
    REQUIRE(proc.st[0] == 0);
    REQUIRE(proc.st[1] == 0);
    REQUIRE(proc.st[2] == 0);
    REQUIRE(proc.st[3] == 0);
    REQUIRE(proc.st[4] == 0);
    REQUIRE(proc.st[5] == 0);
    REQUIRE(proc.st[6] == 0);
    REQUIRE(proc.st[7] == 0);
    //All flags set
    /*
    0000: A0 ...
    */
    reset(&proc);
    memory[0x00] = 0xA0;
    proc.st[0] = 1;
    proc.st[1] = 1;
    proc.st[2] = 1;
    proc.st[3] = 1;
    proc.st[4] = 1;
    proc.st[5] = 1;
    proc.st[6] = 1;
    proc.st[7] = 1;
    proc.cycle();
    REQUIRE(proc.st[0] == 0);
    REQUIRE(proc.st[1] == 0);
    REQUIRE(proc.st[2] == 0);
    REQUIRE(proc.st[3] == 0);
    REQUIRE(proc.st[4] == 0);
    REQUIRE(proc.st[5] == 0);
    REQUIRE(proc.st[6] == 0);
    REQUIRE(proc.st[7] == 0);
}

/* 0xA1 */
TEST_CASE("czf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A1 ...
    */
    reset(&proc);
    memory[0x00] = 0xA1;
    proc.cycle();
    REQUIRE(proc.st[0] == 0);
    //Flag set
    /*
    0000: A1 ...
    */
    reset(&proc);
    memory[0x00] = 0xA1;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.st[0] == 0);
}

/* 0xA2 */
TEST_CASE("szf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A2 ...
    */
    reset(&proc);
    memory[0x00] = 0xA2;
    proc.cycle();
    REQUIRE(proc.st[0] == 1);
    //Flag set
    /*
    0000: A2 ...
    */
    reset(&proc);
    memory[0x00] = 0xA2;
    proc.st[0] = 1;
    proc.cycle();
    REQUIRE(proc.st[0] == 1);
}

/* 0xA3 */
TEST_CASE("cnf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A3 ...
    */
    reset(&proc);
    memory[0x00] = 0xA3;
    proc.cycle();
    REQUIRE(proc.st[1] == 0);
    //Flag set
    /*
    0000: A3 ...
    */
    reset(&proc);
    memory[0x00] = 0xA3;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.st[1] == 0);
}

/* 0xA4 */
TEST_CASE("snf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A4 ...
    */
    reset(&proc);
    memory[0x00] = 0xA4;
    proc.cycle();
    REQUIRE(proc.st[1] == 1);
    //Flag set
    /*
    0000: A4 ...
    */
    reset(&proc);
    memory[0x00] = 0xA4;
    proc.st[1] = 1;
    proc.cycle();
    REQUIRE(proc.st[1] == 1);
}

/* 0xA5 */
TEST_CASE("ccf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A5 ...
    */
    reset(&proc);
    memory[0x00] = 0xA5;
    proc.cycle();
    REQUIRE(proc.st[2] == 0);
    //Flag set
    /*
    0000: A5 ...
    */
    reset(&proc);
    memory[0x00] = 0xA5;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.st[2] == 0);
}

/* 0xA6 */
TEST_CASE("scf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A6 ...
    */
    reset(&proc);
    memory[0x00] = 0xA6;
    proc.cycle();
    REQUIRE(proc.st[2] == 1);
    //Flag set
    /*
    0000: A6 ...
    */
    reset(&proc);
    memory[0x00] = 0xA6;
    proc.st[2] = 1;
    proc.cycle();
    REQUIRE(proc.st[2] == 1);
}

/* 0xA7 */
TEST_CASE("cof", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A7 ...
    */
    reset(&proc);
    memory[0x00] = 0xA7;
    proc.cycle();
    REQUIRE(proc.st[3] == 0);
    //Flag set
    /*
    0000: A7 ...
    */
    reset(&proc);
    memory[0x00] = 0xA7;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.st[3] == 0);
}

/* 0xA8 */
TEST_CASE("sof", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A8 ...
    */
    reset(&proc);
    memory[0x00] = 0xA8;
    proc.cycle();
    REQUIRE(proc.st[3] == 1);
    //Flag set
    /*
    0000: A8 ...
    */
    reset(&proc);
    memory[0x00] = 0xA8;
    proc.st[3] = 1;
    proc.cycle();
    REQUIRE(proc.st[3] == 1);
}

/* 0xA9 */
TEST_CASE("clf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: A9 ...
    */
    reset(&proc);
    memory[0x00] = 0xA9;
    proc.cycle();
    REQUIRE(proc.st[4] == 0);
    //Flag set
    /*
    0000: A9 ...
    */
    reset(&proc);
    memory[0x00] = 0xA9;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.st[4] == 0);
}

/* 0xAA */
TEST_CASE("slf", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: AA ...
    */
    reset(&proc);
    memory[0x00] = 0xAA;
    proc.cycle();
    REQUIRE(proc.st[4] == 1);
    //Flag set
    /*
    0000: AA ...
    */
    reset(&proc);
    memory[0x00] = 0xAA;
    proc.st[4] = 1;
    proc.cycle();
    REQUIRE(proc.st[4] == 1);
}

/* 0xAB */
TEST_CASE("cif", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: AB ...
    */
    reset(&proc);
    memory[0x00] = 0xAB;
    proc.cycle();
    REQUIRE(proc.st[5] == 0);
    //Flag set
    /*
    0000: AB ...
    */
    reset(&proc);
    memory[0x00] = 0xAB;
    proc.st[5] = 1;
    proc.cycle();
    REQUIRE(proc.st[5] == 0);
}

/* 0xAC */
TEST_CASE("sif", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: AC ...
    */
    reset(&proc);
    memory[0x00] = 0xAC;
    proc.cycle();
    REQUIRE(proc.st[5] == 1);
    //Flag set
    /*
    0000: AA ...
    */
    reset(&proc);
    memory[0x00] = 0xAC;
    proc.st[5] = 1;
    proc.cycle();
    REQUIRE(proc.st[5] == 1);
}

/* 0xAD */
TEST_CASE("cef", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    //No flag set
    /*
    0000: AD ...
    */
    reset(&proc);
    memory[0x00] = 0xAD;
    proc.cycle();
    REQUIRE(proc.st[6] == 0);
    //Flag set
    /*
    0000: AD ...
    */
    reset(&proc);
    memory[0x00] = 0xAD;
    proc.st[6] = 1;
    proc.cycle();
    REQUIRE(proc.st[6] == 0);
}

/* 0xB0-0xB5 */
TEST_CASE("inc", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    // 0x00 + 0x01 = 0x01
    /*
    0000: B0 00 10 ...
    */
    reset(&proc);
    memory[0x00] = 0xB0;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(memory[0x10] == 0x01);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    // 0xFF + 0x01 = 0x00 ZNC
    /*
    0000: B1 00 10 ...
    0010: 00 12 FF ...
    */
    reset(&proc);
    memory[0x00] = 0xB1;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x12] = 0xFF;
    proc.cycle();
    REQUIRE(memory[0x12] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    // 0x7F + 0x01 = 0x80 NO
    /*
    0000: B2 01 ...
    */
    reset(&proc);
    memory[0x00] = 0xB2;
    memory[0x01] = 0x01;
    proc.r[1] = 0x7F;
    proc.cycle();
    REQUIRE(proc.r[1] == 0x80);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    // 0x0000 + 0x0001 = 0x0001
    /*
    0000: B3 00 10 ...
    */
    reset(&proc);
    memory[0x00] = 0xB3;
    memory[0x02] = 0x10;
    proc.cycle();
    REQUIRE(memory[0x10] == 0x00);
    REQUIRE(memory[0x11] == 0x01);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    // 0xFFFF + 0x0001 = 0x0000 ZNC
    /*
    0000: B4 00 10 ...
    0010: 00 12 FF FF ...
    */
    reset(&proc);
    memory[0x00] = 0xB4;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    memory[0x12] = 0xFF;
    memory[0x13] = 0xFF;
    proc.cycle();
    REQUIRE(memory[0x12] == 0x00);
    REQUIRE(memory[0x13] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    // 0x7FFF + 0x0001 = 0x8000 NO
    /*
    0000: B5 01 ...
    */
    reset(&proc);
    memory[0x00] = 0xB5;
    memory[0x01] = 0x01;
    proc.r[0] = 0x7F;
    proc.r[1] = 0xFF;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x80);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
}

/* 0xB0-0xB5 */
TEST_CASE("dec", "[opcodes]"){
    cosproc proc = cosproc(MemoryRead, MemoryWrite);
    // 0x01 - 0x01 = 0x00 Z
    /*
    0000: B6 00 10 ...
    0000: 01 ...
    */
    reset(&proc);
    memory[0x00] = 0xB6;
    memory[0x02] = 0x10;
    memory[0x10] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x10] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    // 0x00 - 0x01 = 0xFF NCO
    /*
    0000: B7 00 10 ...
    0010: 00 12 00 ...
    */
    reset(&proc);
    memory[0x00] = 0xB7;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    proc.cycle();
    REQUIRE(memory[0x12] == 0xFF);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    // 0x81 - 0x01 = 0x80 N
    /*
    0000: B8 01 ...
    */
    reset(&proc);
    memory[0x00] = 0xB8;
    memory[0x01] = 0x01;
    proc.r[1] = 0x81;
    proc.cycle();
    REQUIRE(proc.r[1] == 0x80);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    // 0x0001 - 0x0001 = 0x0000 Z
    /*
    0000: B9 00 10 ...
    0000: 00 01 ...
    */
    reset(&proc);
    memory[0x00] = 0xB9;
    memory[0x02] = 0x10;
    memory[0x11] = 0x01;
    proc.cycle();
    REQUIRE(memory[0x10] == 0x00);
    REQUIRE(memory[0x11] == 0x00);
    REQUIRE(proc.st[0] == 1); //Zero
    REQUIRE(proc.st[1] == 0); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
    // 0x0000 - 0x0001 = 0xFFFF NCO
    /*
    0000: BA 00 10 ...
    0010: 00 12 00 00 ...
    */
    reset(&proc);
    memory[0x00] = 0xBA;
    memory[0x02] = 0x10;
    memory[0x11] = 0x12;
    proc.cycle();
    REQUIRE(memory[0x12] == 0xFF);
    REQUIRE(memory[0x13] == 0xFF);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 1); //Carry
    REQUIRE(proc.st[3] == 1); //Overflow
    // 0x8001 - 0x0001 = 0x8000 N
    /*
    0000: BB 01 ...
    */
    reset(&proc);
    memory[0x00] = 0xBB;
    memory[0x01] = 0x01;
    proc.r[0] = 0x80;
    proc.r[1] = 0x01;
    proc.cycle();
    REQUIRE(proc.r[0] == 0x80);
    REQUIRE(proc.r[1] == 0x00);
    REQUIRE(proc.st[0] == 0); //Zero
    REQUIRE(proc.st[1] == 1); //Negative
    REQUIRE(proc.st[2] == 0); //Carry
    REQUIRE(proc.st[3] == 0); //Overflow
}