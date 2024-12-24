#include<iostream>
#include "Terminal.h"

int main(){
  std::cout<<"Hello World"<<std::endl;
  Terminal::get_instance()->render();

return 0;
}
