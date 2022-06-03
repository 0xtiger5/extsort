/**
 * @file sort.cpp
 * @author 0xtiger5 (coder.0xtiger5@foxmail.com)
 * @brief 主函数
 * @version 0.1
 * @date 2022-05-28
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <iostream>
#include <thread>
#include <string>
#include "merge_sort.h"

int main(int argc, char* argv[]) {
    auto app = Merge_Sort(std::string("/data/int16GB.txt"), 5126320, 600);
    // auto app = Merge_Sort(std::string("int"), 32602000, 60);
    // auto app = Merge_Sort(std::string("smallint"), 29343, 10);
    app.advance_merge_sort();
    return 0;
}
