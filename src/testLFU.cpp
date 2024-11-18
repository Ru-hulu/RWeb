#include"MemoryPool.hpp"
#include"LFUcache.hpp"
#include<random>
#include<ctime>
#include<stdlib.h>
int main(int argc, char** argv)
{
    MemoryManager::Initallpool();
    cach_space::LFUcache* this_c = cach_space::getcachpointer();
    std::cout << "the capacity of LFU is : " << this_c->getCapacity() << std::endl;
    std::vector<std::pair<std::string, std::string>> seed(26);
    for(int i = 0; i < 26; ++i) {
    seed[i] = std::make_pair('a' + i, 'A' + i);
    }
    for(int i=0;i<410;i++)
    {
        std::string val;
        // int temp = rand() % 2;//0-25
        int temp = i % 10;//0-25
        // for(std::pair<std::string,std::string> thisp:seed)
        // {
        //     std::cout<<thisp.first<<" "<<thisp.second<<" ";
        // }
        std::cout << "----------round " << i << "----------" << std::endl;
        std::cout<<"wanted key is "<< seed[temp].first<<std::endl;
        if(this_c->get(seed[temp].first,val)==false)
        {
            this_c->set(seed[temp].first,seed[temp].second);
            std::cout<<"get failed set succeed"<<std::endl;
            std::cout<<seed[temp].first<<" "<<seed[temp].second<<" "<<std::endl;
        }
    }
    return 1;
}
