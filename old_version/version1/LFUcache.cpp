#include"LFUcache.hpp" 
#include"MemoryPool.hpp"
namespace cach_space
{
    LFUcache* this_chach = nullptr;//定义
    void KeyList::init(int freq_)
    {
        freq = freq_;
        Dummyhead_ = MemoryManager::newElement<Node<Key>>();
        // Dummyhead_ = new Node<Key>();
        Dummyhead_->set_next(nullptr);
        Dummyhead_->set_pre(nullptr);
        tail_ = Dummyhead_;
    }
    void KeyList::destroy()
    {
        key_node thisn;
        while(Dummyhead_!=nullptr)
        {
            thisn = Dummyhead_->get_next();
            MemoryManager::deleteElement(Dummyhead_);
            // delete Dummyhead_;
            Dummyhead_ = thisn;
        }
        return;
    }

    void KeyList::add(key_node& n)//往这个频率中添加节点
    {
        if(Dummyhead_->get_next()==nullptr)
        {
            tail_ = n;
            Dummyhead_->set_next(n);
            n->set_pre(Dummyhead_);
            n->set_next(nullptr);
        }
        else
        {
            n->set_next(Dummyhead_->get_next());
            n->set_pre(Dummyhead_);
            Dummyhead_->set_next(n);
            n->get_next()->set_pre(n);
        }
            return;
    }

    void KeyList::remove(key_node& n)
    {
        if(n==tail_)
        {
            tail_ = n->get_pre();
            tail_->set_next(nullptr);
        }
        else
        {
            n->get_pre()->set_next(n->get_next());
            n->get_next()->set_pre(n->get_pre());
        }
    }

    bool KeyList::isEmpty()
    {
        if(Dummyhead_==tail_)
        {
            return true;
        }
        return false;
    }

    key_node KeyList::get_tail()
    {
        return tail_;
    }

    LFUcache::LFUcache(int cap_)
    {
        capacity_ = cap_;
        init();
    }
    LFUcache::~LFUcache()
    {
        fre_node fn;
        while(Dummyhead_!=nullptr)
        {
            fn = Dummyhead_->get_next();
            Dummyhead_->get_value().destroy();
            MemoryManager::deleteElement(Dummyhead_);
            // delete Dummyhead_;
            Dummyhead_ = fn;
        }
    }
    void LFUcache::init()
    {
        Dummyhead_ = MemoryManager::newElement<Node<KeyList>>();
        // Dummyhead_ = new Node<KeyList>();
        Dummyhead_->get_value().init(0);
        Dummyhead_->set_next(nullptr);
    }
    void LFUcache::del(fre_node fn)//这里为什么要给引用？
    {
        fn->get_pre()->set_next(fn->get_next());
        if(fn->get_next()!=nullptr)
        {
            fn->get_next()->set_pre(fn->get_pre());
        }
        fn->get_value().destroy();
        //如果fn isEmpty，就是说小链表为空
        //那么destroy需不需要？需要。因为fn里面有一个knode的dummyhead
        MemoryManager::deleteElement(fn);
        // delete fn;
    }
    //这里为什么要给引用？
    //fn是kn当前频率对应的链表。如果kn被访问过1次，则fn.value=1， 如果这是第一次访问，则fn.value = 0;
    void LFUcache::add_Freq(key_node kn,fre_node fn)
    {
        int f_n = fn->get_value().get_freq();
        if(fn!=Dummyhead_)
        {   //如果缓存中已经有当前资源，就是说已经被访问过，则先删除原先缓存。
            fn->get_value().remove(kn);
        }
        if(fn->get_next()==nullptr ||
        fn->get_next()->get_value().get_freq()!=f_n+1)
        {
            fre_node newn = MemoryManager::newElement<Node<KeyList>>();        
            // fre_node newn = new Node<KeyList>();
            newn->get_value().init(f_n+1);
            newn->get_value().add(kn);
            fre_harsh[kn->get_value().key] = newn;
            if(fn->get_next()!=nullptr)
            {
                fn->get_next()->set_pre(newn);
            }
            newn->set_next(fn->get_next());
            fn->set_next(newn);
            newn->set_pre(fn);
        }
        else
        {
            fn->get_next()->get_value().add(kn);
            fre_harsh[kn->get_value().key] = fn->get_next();
        }
        if(fn!=Dummyhead_&&fn->get_value().isEmpty())
        {
                del(fn);
        }//如果原先所在的链表已经空了，删除

    }

    bool LFUcache::get(std::string key,std::string& val)
    {
        if(capacity_<=0)return false;
        std::lock_guard<std::mutex> lck(mtx);
        if(key_harsh.find(key)==key_harsh.end())return false;
        else{val += key_harsh[key]->get_value().value;}
        //读取到信息以后需要修改频率了
        fre_node fn = fre_harsh[key];
        key_node kn = key_harsh[key];
        //std::cout<<"get success freupdate "<<fn->get_value().get_freq()<<" -> ";
        add_Freq(kn,fn);
        //std::cout<<fre_harsh[key]->get_value().get_freq()<<std::endl;
        return true;
    }
    void LFUcache::set(std::string key,std::string val)
    {
        //只有确定一定在缓存中没有，才会调用set
        if(capacity_<=0)return;
        std::lock_guard<std::mutex> lck(mtx);
        if(key_harsh.size()==capacity_)
        {
            fre_node fn = Dummyhead_->get_next();
            key_node kn = fn->get_value().get_tail();
            fn->get_value().remove(kn);//为什么上面一行直接写下来会出错？
            fre_harsh.erase(kn->get_value().key);
            key_harsh.erase(kn->get_value().key);
            MemoryManager::deleteElement(kn);
            // delete kn;
            if(fn->get_value().isEmpty())
            {   //如果小链表是空，删除小链表
                del(fn);
            }
        }
        //如果当前的资源已经用完了，那么就需要删掉一个key_node;
            key_node kn = MemoryManager::newElement<Node<Key>>();
            // key_node kn = new Node<Key>();
            kn->get_value().key = key;
            kn->get_value().value = val;
            add_Freq(kn,Dummyhead_);
            fre_harsh[key] = Dummyhead_->get_next();
            key_harsh[key] = kn;
        //如果当前资源还没有用完，那么直接在头节点后面加上一个key_node
        return;
    }
    LFUcache* getcachpointer()
    {
        this_chach = MemoryManager::newElement<LFUcache>(LFU_CAPACITY);
        // this_chach = new LFUcache(LFU_CAPACITY);
        return this_chach;
    }
}