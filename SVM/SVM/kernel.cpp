#include "kernel.h"

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <limits>

namespace vm
{
    Kernel::Kernel(Scheduler scheduler, std::vector<std::string> executables_paths)
        : machine(), processes(), priorities(), scheduler(scheduler),
          _last_issued_process_id(0),
          _last_ram_position(0),
          _current_process_index(0),
          _cycles_passed_after_preemption(0)
    {
		machine.memory.ram[0] = _free_physical_memory = 0;
		machine.memory.ram[1] = machine.memory.ram.size() - 2;

        std::for_each(executables_paths.begin(), executables_paths.end(), [&](const std::string &path) {
            CreateProcess(path);
        });

        if (scheduler == FirstComeFirstServed || scheduler == ShortestJob) {
            machine.pic.isr_0 = [&]() {
			};

            machine.pic.isr_3 = [&]() {
            };
        } else if (scheduler == RoundRobin) {
            machine.pic.isr_0 = [&]() {
                std::cout << "Kernel: processing the timer interrupt." << std::endl;

                if (!processes.empty()) {
                    if (_cycles_passed_after_preemption <= Kernel::_MAX_CYCLES_BEFORE_PREEMPTION)
                    {
                        std::cout << "Kernel: allowing the current process " << processes[_current_process_index].id << " to run." << std::endl;

                        ++_cycles_passed_after_preemption;

                        std::cout << "Kernel: the current cycle is " << _cycles_passed_after_preemption << std::endl;
                    } else {
                        if (processes.size() > 1) {
                            std::cout << "Kernel: switching the context from process " << processes[_current_process_index].id;
                
                            processes[_current_process_index].registers = machine.cpu.registers;
                            processes[_current_process_index].state = Process::Ready;

                            _current_process_index = (_current_process_index + 1) % processes.size();

                            std::cout << " to process " << processes[_current_process_index].id << std::endl;

                            machine.cpu.registers = processes[_current_process_index].registers;

                            processes[_current_process_index].state = Process::Running;
                        }

                        _cycles_passed_after_preemption = 0;
                    }
                }

                std::cout << std::endl;
            };

            machine.pic.isr_3 = [&]() {
                std::cout << "Kernel: processing the first software interrupt." << std::endl;

                if (!processes.empty()) {
                    std::cout << "Kernel: unloading the process " << processes[_current_process_index].id << std::endl;
					FreePhysicalMemory(processes[_current_process_index].memory_start_position);
                    processes.erase(processes.begin() + _current_process_index);

                    if (processes.empty()) {
                        _current_process_index = 0;

                        std::cout << "Kernel: no more processes. Stopping the machine." << std::endl;

                        machine.Stop();
                    } else {
                        if (_current_process_index >= processes.size()) {
                            _current_process_index %= processes.size();
                        }

                        std::cout << "Kernel: switching the context to process " << processes[_current_process_index].id << std::endl;

                        machine.cpu.registers = processes[_current_process_index].registers;
						// TODO: switch the page table in MMU to the table of the current process

                        processes[_current_process_index].state = Process::Running;

                        _cycles_passed_after_preemption = 0;
                    }
                }

                std::cout << std::endl;
            };
        } else if (scheduler == Priority) {
            machine.pic.isr_0 = [&]() {
			};

            machine.pic.isr_3 = [&]() {
            };
        }

        machine.Start();
    }

    Kernel::~Kernel() {}

    void Kernel::CreateProcess(const std::string &name)
    {
        if (_last_issued_process_id == std::numeric_limits<Process::process_id_type>::max()) {
            std::cerr << "Kernel: failed to create a new process. The maximum number of processes has been reached." << std::endl;
        } else {
            std::ifstream input_stream(name, std::ios::in | std::ios::binary);
            if (!input_stream) {
                std::cerr << "Kernel: failed to open the program file." << std::endl;
            } else {
                Memory::ram_type ops;

                input_stream.seekg(0, std::ios::end);
                auto file_size = input_stream.tellg();
                input_stream.seekg(0, std::ios::beg);
                ops.resize(static_cast<Memory::ram_size_type>(file_size) / 4);

                input_stream.read(reinterpret_cast<char *>(&ops[0]), file_size);

                if (input_stream.bad()) {
                    std::cerr << "Kernel: failed to read the program file." << std::endl;
                } else {

					Memory::ram_size_type allocated_index = Kernel::AllocatePhysicalMemory(ops.size());
					if( allocated_index == NULL){
						std::cout <<"Kernel: There is not enough Physical Memory !!!" << std::endl;	
					}


					std::copy(ops.begin(), ops.end(), (machine.memory.ram.begin() + allocated_index));

					Process process(_last_issued_process_id++, allocated_index - 2,
                                                               allocated_index + ops.size());
                    
					processes.push_back(process);
					machine.cpu.registers = processes.front().registers;

                    
                }
            }
        }
    }


	Memory::ram_size_type Kernel::AllocatePhysicalMemory(Memory::ram_size_type units) //first
		
		Memory::ram_size_type prev = _free_physical_memory;
		Memory::ram_size_type current_index;


		for(Memory::ram_size_type next_free = machine.memory.ram[prev]; ;  prev = next_free, next_free = machine.memory.ram[next_free])
		{
			Memory::ram_size_type size = machine.memory.ram[next_free + 1];
			if( size >= units)
			{
				if(size == units)
				{
					machine.memory.ram[prev] = machine.memory.ram[next_free];
				}
				else
				{
					machine.memory.ram[next_free + 1] -= units + 2;
					next_free +=  machine.memory.ram[next_free + 1];	

					machine.memory.ram[next_free + 1] = units;
				}
				_free_physical_memory = prev;

				return next_free + 2;

			}
			if(next_free == _free_physical_memory)
			{
				return NULL;
			}
		}


		return 0;
	}


	void Kernel::FreePhysicalMemory(Memory::ram_size_type index ) // second
	{
		Memory::ram_size_type previous_free_block_index = _free_physical_memory;
		Memory::ram_size_type current_block_index = index;

		for(; (current_block_index > previous_free_block_index && current_block_index < machine.memory.ram[previous_free_block_index]);
			   previous_free_block_index = machine.memory.ram[previous_free_block_index])																
		{
			if( previous_free_block_index> machine.memory.ram[previous_free_block_index] && 
					((current_block_index > previous_free_block_index || current_block_index < machine.memory.ram[previous_free_block_index]))){
						break;
			}
				
		}

		if(current_block_index + machine.memory.ram[current_block_index + 1] == machine.memory.ram[previous_free_block_index]){

			machine.memory.ram[current_block_index + 1] += machine.memory.ram[previous_free_block_index + 1];

			machine.memory.ram[current_block_index] = machine.memory.ram[machine.memory.ram[previous_free_block_index]];

		} else {

			machine.memory.ram[current_block_index] = machine.memory.ram[previous_free_block_index];
		}
		if(previous_free_block_index + machine.memory.ram[previous_free_block_index + 1] == current_block_index){

			machine.memory.ram[previous_free_block_index + 1] += machine.memory.ram[current_block_index + 1];
			machine.memory.ram[previous_free_block_index] = machine.memory.ram[ current_block_index];

		}else{
			machine.memory.ram[previous_free_block_index] = current_block_index;
		}

		_free_physical_memory = previous_free_block_index;


		
		
	}
}
