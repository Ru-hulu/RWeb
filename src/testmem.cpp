#include"MemoryPool.hpp"
class Person
{
    public:
     int id_;
     std::string name_;
     Person(int id,std::string name):id_(id),name_(name){
        printf("构造函数调用");
     };
     ~Person(){
        std::cout<<"析构函数调用"<<std::endl;
     };
};
void test01()
{
    MemoryManager::Initallpool();
    std::shared_ptr<Person> p1(MemoryManager::newElement<Person>(1,"zhangsan"),MemoryManager::deleteElement<Person>);
    // std::shared_ptr<Person> p1(new Person(1,"zhangsan"));
    printf("sizeof name %d \n",sizeof(p1->name_));
    printf("sizeof Person %d \n",sizeof(Person));
}
int main(int argc, char** argv)
{
    test01();
    return 0;
}
