void printMemoryUsage() {
        lockAllocator();
        MemoryBlock* current = freeList;
        size_t used = 0, totalFree = 0;
        int blockNum = 0;
        cout << "\n Memory Pool Status:\n";
        cout << "-----------------------------------------------\n";
        while (current) {
            cout << "Block #" << ++blockNum << " | Addr: " << current
                      << " | Size: " << std::setw(4) << current->size
                      << " bytes | Free: " << (current->free ? "Yes" : "No") << "\n";
            if (current->free) totalFree += current->size;
            else used += current->size;
            current = current->next;
        }
        cout << "-----------------------------------------------\n";
        cout << " Total used: " << used << " bytes | Free: " << totalFree << " bytes\n";
        cout << "-----------------------------------------------\n";
        unlockAllocator();
    }