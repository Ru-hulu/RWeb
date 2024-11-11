#include"MemoryPool.hpp"
struct Slot
{
    Slot* next;
};
class MemoryPool
{
    public:
        Slot* allocate();
        void deallocate(void* p);
};
Slot* MemoryPool::allocate()
{
    return nullptr;
}
void MemoryPool::deallocate(void* p)
{

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