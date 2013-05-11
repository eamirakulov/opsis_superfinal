#include "cpu.h"

#include <iostream>

namespace vm
{
    Registers::Registers()
        : a(0), b(0), c(0), flags(0), ip(0), sp(0) {}

    CPU::CPU(Memory &memory, PIC &pic): registers(), _memory(memory), _pic(pic) {}

    CPU::~CPU() {}

    void CPU::Step()
    {
        int ip = registers.ip;

        int instruction = _memory.ram[ip];
        int data = _memory.ram[ip + 1];

        if (instruction == CPU::MOVA_BASE_OPCODE) {
            registers.a = data;
            registers.ip += 2;
        } else if (instruction == CPU::MOVB_BASE_OPCODE) {
            registers.b = data;
            registers.ip += 2;
        } else if (instruction == CPU::MOVC_BASE_OPCODE) {
            registers.c = data;
            registers.ip += 2;
		} else if (instruction == CPU::STA_BASE_OPCODE) {
			_memory.ram[data] = registers.a;
			registers.ip += 2;
		} else if (instruction == CPU::STB_BASE_OPCODE) {
			_memory.ram[data] = registers.b;
			registers.ip += 2;
		} else if (instruction == CPU::STC_BASE_OPCODE) {
			_memory.ram[data] = registers.c;
		} else if (instruction == CPU::JMP_BASE_OPCODE) {
            registers.ip += data;
        } else if (instruction == CPU::INT_BASE_OPCODE) {
            _pic.isr_3();
        } else {
            std::cerr << "CPU: invalid opcode data. Skipping..." << std::endl;
            registers.ip += 2;
        }
    }
}
