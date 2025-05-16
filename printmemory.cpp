void printMemoryMap() {
        lockAllocator();
        MemoryBlock* current = freeList;
        cout << "Memory Map:\n[";
        const int barLength = 40;
        size_t totalUsed = 0;

        while (current) {
            float blockRatio = static_cast<float>(current->size) / poolSize;
            int symbols = std::max(1, static_cast<int>(blockRatio * barLength));
            char symbol = current->free ? '-' : '#';
            for (int i = 0; i < symbols; ++i) std::cout << symbol;
            if (!current->free) totalUsed += current->size;
            current = current->next;
        }

        cout << "\n Used: " << totalUsed << " / " << poolSize << " bytes\n";
        unlockAllocator();
    }