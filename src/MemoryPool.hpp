#include<iostream>
#include<memory>
#include<mutex>
#ifndef MEMORYPOOL_HPP
#define MEMORYPOOL_HPP
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
            void deallocate(Slot* p);
            Slot* currentBlock_;//当前的块
            Slot* currentSlot_;//当前可以分配的槽
            Slot* begin_slot;            
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
    extern void* use_Memory(size_t s);
    extern void free_Memory(size_t s, void* p);
    extern void Initallpool();
    template<typename T,typename... Args>
    T* newElement(Args&&... args)//万能引用
    {
        //先向内存池获得空间
        T* p = reinterpret_cast<T*>(use_Memory(sizeof(T)));
        // std::cout<<sizeof(T)<<std::endl;
        if(p!=nullptr)
        {
            new(p) T(std::forward<Args>(args)...);//完美转发
        }
        return p;
    }
    template<typename T>
    void deleteElement(T* p)
    {
        if(p!=nullptr)
        {
            p->~T();//先对对象进行析构
            free_Memory(sizeof(p),reinterpret_cast<void*>(p));
            //再将内存还给内存池
        }
    }
}
#endif