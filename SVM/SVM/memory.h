#ifndef MEMORY_H
#define MEMORY_H

#include <vector>

namespace vm
{
    class Memory
    {
    public:
        typedef std::vector<int> ram_type;
        typedef ram_type::size_type ram_size_type;

        static const ram_size_type DEFAULT_RAM_SIZE = 0xFFFF;

        ram_type ram;

        Memory();
        virtual ~Memory();
    };
}

#endif
