#include"MemoryPool.hpp"
#include<mutex>
#define BlockSize 4096
struct Slot
{
    Slot* next;
};
class MemoryPool
{
    public:
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
size_t MemoryPool::Caculatepadding(size_t align,char* nowp)
{
    size_t result = reinterpret_cast<size_t>(nowp);
    return (align - result)%result;
}
//现在需要创造一个新的Block
Slot* MemoryPool::createnewBlock()
{
    char* nowBlock = reinterpret_cast<char*>(operator new(BlockSize));
    //先分配得到空间
    reinterpret_cast<Slot*>(nowBlock)->next = currentBlock_;
    currentBlock_ = reinterpret_cast<Slot*>(nowBlock);
    char* body = nowBlock + sizeof(Slot);
    size_t paddingsize = Caculatepadding(sizeof(slot_size),body);
    currentSlot_ = reinterpret_cast<Slot*>(body + paddingsize);
    lastslot_ = reinterpret_cast<Slot*>(nowBlock + BlockSize - slot_size + 1);
    Slot* nowslot = currentSlot_;
    currentSlot_ += (slot_size>>3);    
    return nowslot;
    //Block链表维护好
}
//这说明free链表中已经没有可以使用的内存了
Slot* MemoryPool::nofree_solve()
{
    std::unique_lock<std::mutex> this_l(nofree_mutex); 
    if(freeslot_<lastslot_)//说明Block中还有空间可以用
    {
            Slot* nowslot = currentSlot_;
            currentSlot_ += (slot_size<<3);
            return nowslot;
    }
    //这个说明当前block也没有内存可以用了
    return createnewBlock();
}
Slot* MemoryPool::allocate()
{
    if(freeslot_!=nullptr)
    {
        std::lock_guard<std::mutex> this_g(free_mutex);
        if(freeslot_!=nullptr)
        {
            Slot* nowslot = freeslot_;
            freeslot_ = freeslot_->next;
            return nowslot;
        }
    }
    return nofree_solve();
}
void MemoryPool::deallocate(void* p)
{
    if(p==nullptr)return;
    std::lock_guard<std::mutex> this_guard(free_mutex);
    Slot* thisp = reinterpret_cast<Slot*>(p);
    thisp->next = freeslot_;
}

MemoryPool& getMemoryPool(size_t s)
{
    static MemoryPool allpools[64];
    int mid = ((s+7) >> 3) -1;
    return allpools[mid];    
}

void* use_Memory(size_t s)
{
    if(s>512)
    {
        return operator new(s);
    }
    else
    {
        if(s<=0)return nullptr;
        else
        {
            return reinterpret_cast<void*>(getMemoryPool(s).allocate());
        }
    }
    //直接向内存池申请空间的函数
}
void free_Memory(size_t s, void* p)
{
    if(p==nullptr)return;
    if(s>512) delete p;//如果太大直接释放
    else//如果不是很大，就还给内存池
    {
        getMemoryPool(s).deallocate(reinterpret_cast<Slot*>(p));
    }
    //将内存还给内存池的函数
}
template<typename T,typename... Args>
T* newElement(Args&&... args)//万能引用
{
    //先向内存池获得空间
    T* p = reinterpret_cast<T*>(use_Memory(sizeof(T)));
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
int main()
{

    return 0;
}