#include <iostream>
#include <vector>
#include <cstdlib>
#include <new> 
#include <cstring>
#include <atomic>
#include <iomanip>
#include <chrono>
#ifdef _WIN32
#include <malloc.h>
#include <regex>
#include <map>
#endif
#define ALIGNMENT 64
using namespace std;

enum Strategy { FIRST_FIT, BEST_FIT, WORST_FIT };

struct MemoryBlock {
    size_t size;
    bool free;
    MemoryBlock* next;
};

class SmartAllocator {
private:
    void* memoryPool;
    MemoryBlock* freeList;
    size_t poolSize;
    Strategy strategy;
    std::atomic_flag lock = ATOMIC_FLAG_INIT;

    void coalesce() {
        MemoryBlock* current = freeList;
        while (current && current->next) {
            if (current->free && current->next->free) {
                current->size += sizeof(MemoryBlock) + current->next->size;
                current->next = current->next->next;
                std::cout << " Coalesced adjacent free blocks.\n";
            } else {
                current = current->next;
            }
        }
    }

    void lockAllocator() {
        while (lock.test_and_set(memory_order_acquire));
    }

    void unlockAllocator() {
        lock.clear(memory_order_release);
    }

    MemoryBlock* findBlock(size_t size) {
        MemoryBlock* current = freeList;
        MemoryBlock* selected = nullptr;

        while (current) {
            __builtin_prefetch(current->next, 0, 3);
            if (current->free && current->size >= size) {
                if (strategy == FIRST_FIT) return current;

                if (!selected) selected = current;
                else if (strategy == BEST_FIT && current->size < selected->size) selected = current;
                else if (strategy == WORST_FIT && current->size > selected->size) selected = current;
            }
            current = current->next;
        }
        return selected;
    }

public:
    SmartAllocator(size_t size, Strategy strat = FIRST_FIT) : strategy(strat) {
        poolSize = size;
#ifdef _WIN32
        void* ptr = _aligned_malloc(poolSize, ALIGNMENT);
        if (!ptr) throw std::bad_alloc();
#else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, ALIGNMENT, poolSize) != 0) {
            throw std::bad_alloc();
        }
#endif
        memoryPool = ptr;
        freeList = static_cast<MemoryBlock*>(memoryPool);
        freeList->size = poolSize - sizeof(MemoryBlock);
        freeList->free = true;
        freeList->next = nullptr;
        std::cout << "SmartAllocator initialized with pool size: " << poolSize << " bytes\n";
    }

    void setStrategy(Strategy strat) {
        strategy = strat;
    }

    void* allocate(size_t size) {
        lockAllocator();
        std::cout << "\n Allocating " << size << " bytes...\n";
        MemoryBlock* block = findBlock(size);

        if (!block) {
            std::cout << " Allocation failed! No suitable block found.\n";
            unlockAllocator();
            return nullptr;
        }

        if (block->size > size + sizeof(MemoryBlock)) {
            MemoryBlock* newBlock = reinterpret_cast<MemoryBlock*>(reinterpret_cast<char*>(block) + sizeof(MemoryBlock) + size);
            newBlock->size = block->size - size - sizeof(MemoryBlock);
            newBlock->free = true;
            newBlock->next = block->next;
            block->next = newBlock;
            block->size = size;
        }

        block->free = false;
        cout << " Allocated at address: " << (void*)(block + 1) << "\n";
        unlockAllocator();
        return reinterpret_cast<void*>(block + 1);
    }

    void deallocate(void* ptr) {
        if (!ptr) return;
        lockAllocator();
        MemoryBlock* block = reinterpret_cast<MemoryBlock*>(ptr) - 1;
        block->free = true;
        cout << "\n Deallocated memory at address: " << ptr << "\n";
        coalesce();
        unlockAllocator();
    }




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

    ~SmartAllocator() {
#ifdef _WIN32
        _aligned_free(memoryPool);
#else
        free(memoryPool);
#endif
    }
};

void testStrategy(SmartAllocator& allocator, const std::string& name) {
    cout << "\n=== Testing " << name << " ===\n";
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i) {
        ptrs.push_back(allocator.allocate(100 + (i % 3) * 50));
    }
    for (int i = 0; i < ptrs.size(); i += 2) {
        allocator.deallocate(ptrs[i]);
    }
    allocator.printMemoryUsage();
    allocator.printMemoryMap();
}
void simulateCodeInput(SmartAllocator& allocator) {
    std::cin.ignore();
    std::string line;
    std::vector<std::string> codeLines;
    std::map<std::string, void*> symbolTable;

    std::cout << "\nEnter your C++ code (type END to finish):\n";

    while (true) {
        std::getline(std::cin, line);
        if (line == "END") break;
        codeLines.push_back(line);
    }

    std::regex varPattern(R"((int|char|double|float)\s+([a-zA-Z_]\w*)(\[(\d+)\])?\s*;)");
    for (const auto& code : codeLines) {
        std::smatch match;
        if (std::regex_search(code, match, varPattern)) {
            std::string type = match[1];
            std::string varName = match[2];
            int arraySize = match[4].matched ? std::stoi(match[4]) : 1;

            size_t typeSize = 0;
            if (type == "int") typeSize = sizeof(int);
            else if (type == "double") typeSize = sizeof(double);
            else if (type == "float") typeSize = sizeof(float);
            else if (type == "char") typeSize = sizeof(char);

            size_t totalSize = arraySize * typeSize;
            void* addr = allocator.allocate(totalSize);
            if (addr) {
                symbolTable[varName] = addr;
                std::cout << " Variable '" << varName << "' of type '" << type
                          << "' allocated " << totalSize << " bytes at address " << addr << "\n";
            }
        }
    }

    allocator.printMemoryUsage();
    allocator.printMemoryMap();
}
int main() {
    const size_t POOL_SIZE = 2048;
    SmartAllocator allocator(POOL_SIZE);

    std::vector<void*> allocations;
    int choice;

    while (true) {
        cout << "\n=== Smart Memory Allocator ===\n";
        cout << "1. Allocate Memory\n2. Deallocate Memory\n3. Show Memory Usage\n4. Change Strategy\n5. Compare Strategies & Exit\nEnter choice: ";
        cin >> choice;

        if (choice == 1) {
            size_t size;
            std::cout << "Enter size to allocate: ";
            std::cin >> size;
            void* ptr = allocator.allocate(size);
            if (ptr) allocations.push_back(ptr);
            allocator.printMemoryMap();
        } else if (choice == 2) {
            int index;
            std::cout << "Enter allocation index to deallocate (0 - " << allocations.size() - 1 << "): ";
            std::cin >> index;
            if (index >= 0 && index < allocations.size() && allocations[index]) {
                allocator.deallocate(allocations[index]);
                allocations[index] = nullptr;
            } else {
                std::cout << " Invalid index or already deallocated.\n";
            }
        } else if (choice == 3) {
            allocator.printMemoryUsage();
        } else if (choice == 4) {
            int strat;
            std::cout << "Select strategy (1 = First Fit, 2 = Best Fit, 3 = Worst Fit): ";
            std::cin >> strat;
            if (strat == 1) allocator.setStrategy(FIRST_FIT);
            else if (strat == 2) allocator.setStrategy(BEST_FIT);
            else if (strat == 3) allocator.setStrategy(WORST_FIT);
            std::cout << "Strategy changed.\n";
        } 
        else if (choice == 5) {
            SmartAllocator firstFitAllocator(POOL_SIZE, FIRST_FIT);
            SmartAllocator bestFitAllocator(POOL_SIZE, BEST_FIT);
            SmartAllocator worstFitAllocator(POOL_SIZE, WORST_FIT);

            testStrategy(firstFitAllocator, "First Fit");
            testStrategy(bestFitAllocator, "Best Fit");
            testStrategy(worstFitAllocator, "Worst Fit");

            cout << "\nExiting program.\n";
            break;
        }
        else if (choice == 6) 
        {
            simulateCodeInput(allocator);
        }
        else 
        {
            cout << "\n Invalid option. Try again.\n";
        }
    }
    return 0;
}
