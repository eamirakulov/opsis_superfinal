#ifndef KERNEL_H
#define KERNEL_H

#include <deque>
#include <queue>
#include <string>

#include "machine.h"
#include "process.h"

namespace vm
{
    class Kernel
    {
    public:
        enum Scheduler
        {
            FirstComeFirstServed,
            ShortestJob,
            RoundRobin,
            Priority
        };

        typedef std::deque<Process> process_list_type;
        typedef std::priority_queue<Process> process_priorities_type;

        Machine machine;

        process_list_type processes;
        process_priorities_type priorities;

        Scheduler scheduler;

        Kernel(Scheduler scheduler, std::vector<std::string> executables_paths);
        virtual ~Kernel();

		Memory::ram_size_type AllocatePhysicalMemory(Memory::ram_size_type units);

		void FreePhysicalMemory(Memory::ram_size_type index);
        void CreateProcess(const std::string &name);

    private:
        static const unsigned int _MAX_CYCLES_BEFORE_PREEMPTION = 100;

        Process::process_id_type _last_issued_process_id;
        Memory::ram_type::size_type _last_ram_position;
		Memory::ram_size_type _free_physical_memory;

        unsigned int _cycles_passed_after_preemption;
        process_list_type::size_type _current_process_index;
    };
}

#endif
