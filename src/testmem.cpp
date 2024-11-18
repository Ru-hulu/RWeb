#include"MemoryPool.hpp"
#include<vector>
class Person
{
    public:
     int id_;
     Person(int id):id_(id){
        printf("构造函数调用");
     };
     ~Person(){
        std::cout<<"析构函数调用"<<std::endl;
     };
};
class Person1
{
    public:
     int id_[2]={0};
     Person1(){
        printf("构造函数调用");
     };
     ~Person1(){
        std::cout<<"析构函数调用"<<std::endl;
     };
};
class Person2
{
    public:
     std::string thiss;
     int d = 2;
     Person2(){
        printf("构造函数调用");
     };
     ~Person2(){
        std::cout<<"析构函数调用"<<std::endl;
     };
};
class Person3
{
    public:
     int d = 2;
     char c = 'k';
     Person3(){
        printf("构造函数调用");
     };
     ~Person3(){
        std::cout<<"析构函数调用"<<std::endl;
     };
};
class Person4
{
    public:
     double d = 2;
     char c = 'k';
};


template<int N>
class Person5
{
    public:
      int d[N]={0};
    //  char c = 'k';
};
int main(int argc, char** argv)
{

   std::vector<Person5<5>*> p5s5;
   for(int i=0;i<4096;i++)
   {
      Person5<5>* p = reinterpret_cast<Person5<5>*> (MemoryManager::newElement<Person5<5>>());
      p5s5.push_back(p);
   }


   for(int i=0;i<4096;i++)
   {   
      MemoryManager::deleteElement<Person5<5>>(p5s5[i]);
   }
 
    return 0;
}
