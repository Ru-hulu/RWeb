#include<iostream>
#include<memory>
#include<mutex>
#define BlockSize 4096
struct Slot
{
    Slot* next;
};
namespace MemoryManager
{
    class MemoryPool
    {
        public:
            MemoryPool();
            void init(size_t s);
            Slot* allocate();
            Slot* nofree_solve();
            void deallocate(void* p);
            Slot* currentBlock_;//当前的块
            Slot* currentSlot_;//当前可以分配的槽
            Slot* freeslot_;//链表
            Slot* lastslot_;//最后标记位
            Slot* createnewBlock();
            size_t Caculatepadding(size_t align,char* nowp);
        private:
            size_t slot_size;//cao的大小
            std::mutex free_mutex;
            std::mutex nofree_mutex;
            std::mutex other_mutex;
    };
    extern MemoryPool allpools[64];
    void Initallpool();
}