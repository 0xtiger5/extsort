#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>

int main() {
    auto s = std::string("int.txt");
    std::ifstream in(s);
    std::string temp;
    std::getline(in, temp);
    printf(temp.c_str());
}