#include"MemoryPool.hpp"
namespace MemoryManager
{

    MemoryPool allpools[64];
    MemoryPool& getMemoryPool(size_t s)
    {
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
    void Initallpool()
    {
        size_t slot_size=0;
        for(int i=1;i<=64;i++)
        {
            slot_size += 8;
            getMemoryPool(slot_size).init(slot_size);
        }
    }

    MemoryPool::MemoryPool(){};
    void MemoryPool::init(size_t s)
    {
        slot_size = s;
        currentBlock_ = nullptr;
        currentSlot_ = nullptr;
        freeslot_ = nullptr;
        lastslot_ = nullptr;
    }
    size_t MemoryPool::Caculatepadding(size_t align,char* nowp)
    {
        size_t result = reinterpret_cast<size_t>(nowp);
        return (align - result)%align;
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
        begin_slot = currentSlot_;
        lastslot_ = reinterpret_cast<Slot*>(nowBlock + BlockSize - slot_size + 1);
        Slot* nowslot = currentSlot_;
        currentSlot_ += (slot_size>>3);    
        return nowslot;
        //Block链表维护好
    }
    //这说明free链表中已经没有可以使用的内存了
    Slot* MemoryPool::nofree_solve()
    {
                std::cout<<"slotsize is "<<slot_size<<std::endl;
                std::cout<<"currentslot is "<<currentSlot_<<std::endl;
                std::cout<<"freeslot is "<<freeslot_<<std::endl;
                std::cout<<"lastslot is "<<lastslot_<<std::endl;
                std::cout<<"begin_slot is "<<begin_slot<<std::endl;
        std::unique_lock<std::mutex> this_l(nofree_mutex); 
        if(freeslot_<lastslot_)//说明Block中还有空间可以用
        {
                Slot* nowslot = currentSlot_;
                currentSlot_ += (slot_size>>3);
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

    void MemoryPool::deallocate(Slot* p)
    {
        if(p==nullptr)return;
        std::lock_guard<std::mutex> this_guard(free_mutex);
        p->next = freeslot_;
        freeslot_ = p;
    }
}
