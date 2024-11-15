#include<iostream>
#include<memory>
#include<mutex>
#include<unordered_map>
template <typename T>
class Node
{
    public:
        Node* get_pre(){return pre_node;};
        Node* get_next(){return next_node;};
        Node* set_pre(Node *p){pre_node = p;};
        Node* set_next(Node *p){next_node = p;};
        T& get_value(){return value;};

    private:
        T value;
        Node* pre_node;
        Node* next_node;
};

struct Key
{
    std::string key;
    std::string value;
};
typedef Node<Key>* key_node;

class KeyList
{
    public:
        void init(int freq_);
        void destroy();//删除所有节点
        int get_freq(){return freq;};//得到频率
        void add(key_node& n);//往这个频率中添加节点
        void remove(key_node& n);//删除这个频率中的某个节点
        bool isEmpty();//是否为空
        key_node get_tail();//得到该频率中最后一个节点
        
    private:
        int freq;
        key_node Dummyhead_;
        key_node tail_;
};
typedef Node<KeyList>* fre_node;

class LFUcache
{
    public:
        LFUcache(int cap_);
        ~LFUcache();
        size_t getCapacity(){return capacity_;};
        bool get(std::string key,std::string& val);//找key对应的内容
        void set(std::string key,std::string val);//设置key对应的内容

    private:
        size_t capacity_;
        fre_node Dummyhead_;
        std::mutex mtx;
        std::unordered_map<std::string,key_node> key_harsh;
        std::unordered_map<std::string,fre_node> fre_harsh;
        void add_Freq(key_node kn,fre_node fn);//这里为什么要给引用？
        void del(fre_node fn);//这里为什么要给引用？ 这里释放了空间
        void init();
};