#include <cpu/msr.h>
#include <stdint.h>

uint64_t rdmsr(uint8_t reg){
    uint32_t edx = 0, eax = 0;
    __asm__ volatile ("rdmsr\n\t" : "=d" (edx), "=a" (eax) : "c"(reg));
    return ((uint64_t)edx << 32) | eax;
}

void wrmsr(uint8_t reg, uint64_t value){
    __asm__ volatile("wrmsr\n\t" :: "c" ((uint32_t)reg), "d" (value >> 32), "a" ((uint32_t)value));
}