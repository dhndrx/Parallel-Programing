#include<iostream>
#include <string>

int main(int argc, char**argv){
  std::string message = "hello";

  if(message == argv[1])
    std::cout<<std::endl<<"you said hello\n";
  return 0;
}
