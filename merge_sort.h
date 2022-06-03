/**
 * @file merge_sort.h
 * @author 0xtiger5 (coder.0xtiger5@foxmail.com)
 * @brief 归并排序类
 * @version 0.1
 * @date 2022-05-29
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <map>
#include <utility>
#include <mutex>
#include <unistd.h>

#define N 6
#define machine
#ifdef machine
#define CacheCount 89958   // 一页是989544B，temp文件中的一条数据是11B，大概一页有89958条数据（不知道这么算对不对）
#else
#define CacheCount 2159944  //我的云主机是这个
#endif

std::vector<std::string> pre_process(std::vector<int>& mem, std::ifstream& in, const int num);
struct cmp {
    bool operator() (std::pair<int, int>& a, std::pair<int, int>& b) {
        return a.first > b.first;
    }
};


class Merge_Sort {

private:
    std::vector<int> memory;    //共享内存数组，多线程未用到
    const std::string srcFile;  //源文件位置
    const unsigned LEN;         //每一个块的大小
    const unsigned NUM;         //块的数量，完全没用到，起记录作用

public:
    Merge_Sort(std::string f, unsigned l, unsigned n) : srcFile(f), LEN(l), NUM(n) { }

    ~Merge_Sort() { }

    void merge_sort();
    void advance_merge_sort();
};

// 未做什么优化的版本
void Merge_Sort::merge_sort() {
    //读文件，预处理排序。
    std::ifstream in(this->srcFile);
    auto files = pre_process(this->memory, in, this->LEN);

    //归并，输出
    auto out = std::ofstream(std::string("result.txt"));

    auto readFiles = std::vector<std::ifstream>();
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, cmp> buf; //用于取出归并时最大值的优先队列
    for (auto i : files) {
        readFiles.push_back(std::ifstream(i));
    }
    int available = readFiles.size();

    printf("Start to merge the temp files...\n");
    //从多个文件中，每个文件读出一个数放在优先队列里，如果有一个文件读完了，就将可用数-1
    for (int i = 0; i < available; i++) {
        std::string temp;
        if (std::getline(readFiles[i], temp)) {
            buf.push(std::pair<int, int>(std::stoi(temp), i));
        }
        else {
            available--;
        }
    }
    int count = 0;
    const char* lable = "|/-\\";
    do {
        //从缓冲区中取出最小的数
        int num = buf.top().first;
        int pos = buf.top().second;

        //写入文件 
        std::string s = std::to_string(num) + "\n";
        out.write(s.c_str(), s.length());
        buf.pop();
        count++;
        if (count % 500 == 0) {
            // printf("\rAlready write %d numbers...[%c]", count, lable[(count / 200) % 4]);
        }

        //在相应的文件中再读一个数，如果读不出来，就将可用数-1
        std::string temp;
        if (std::getline(readFiles[pos], temp)) {
            buf.push(std::pair<int, int>(std::stoi(temp), pos));
        }
        else {
            available--;
        }
        // 直到available == 0，没有可以读出的文件。
    } while (available > 0);
    // 关于优先队列与可用文件数，满足这样的不变性：
    // 如果优先队列非空，那么一定有可用的文件。
    printf("\nCompelete merge-sorting........%ld\n", buf.size());
}

std::vector<std::string> pre_process(std::vector<int>& mem, std::ifstream& in, const int num) {
    //读入缓冲区
    std::string line;
    int count = 0;
    int order = 1;
    std::vector<std::string> ans;
    printf("Chuncking the whole src file...\n");
    char lable[4] = { '|', '/', '-', '\\' };
    while (std::getline(in, line)) {
        if (count >= num) {
            // printf("The %dth(st/nd/rd) block, sorting...\t\t\r", order);
            std::sort(mem.begin(), mem.end());
            std::string filename(std::string("temp") + std::string(std::to_string(order)));
            auto out = std::ofstream(filename);
            // printf("The %dth(st/nd/rd) block, writing to the temp file...\t\t\r", order);
            for (auto n : mem) {
                out.write(std::to_string(n).c_str(), std::to_string(n).length());
                out.write("\n", 1);
            }
            out.close();
            order++;
            ans.push_back(filename);
            count = 0;
            mem.clear();
        }
        // printf("%s: %d\n", line.c_str(), std::stoi(line));
        mem.push_back(std::stoi(line));
        count++;
    }
    // 处理最后一部分
    printf("The last block, sorting...\r");
    std::sort(mem.begin(), mem.end());
    std::string filename(std::string("temp") + std::string(std::to_string(order)));
    auto out = std::ofstream(filename);
    auto size = mem.size();
    printf("The last block, writing to the temp file...\r");
    for (auto n : mem) {
        out.write(std::to_string(n).c_str(), std::to_string(n).length());
        out.write("\n", 1);
        // printf("%s", (std::to_string(n) + "\n").c_str());
    }
    out.close();
    ans.push_back(filename);
    mem.clear();
    printf("Compelete chuncking the whole src file and sorting the temp files.\r\n");
    return ans;
}

void thread_run(std::ifstream& in, std::mutex& inMutex, unsigned order, const unsigned max, std::vector<std::string>& tempfiles, std::mutex& outMutex) {
    std::vector<int> buf(max);
    std::size_t count = 0;
    std::string line;
    inMutex.lock();
    while (true) {
        // 从大文件读一个数
        auto& res = std::getline(in, line);
        if (res) {
            if (count == 0) printf("Reading data to %d block\n", order);
            buf[count] = std::stoi(line);
            count++;
        }
        else {
            inMutex.unlock();
            // 读不到数了，清空缓冲区
            if (count != 0) {
                printf("Sorting the %d block.\n", order);
                std::sort(buf.begin(), buf.begin() + count);
                std::string filename(std::string("temp") + std::string(std::to_string(order)));
                auto out = std::ofstream(filename);
                printf("Outputing the %d block.\n", order);
                for (int i = 0; i < count; i++) {
                    int n = buf[i];
                    out.write(std::to_string(n).c_str(), std::to_string(n).length());
                    out.write("\n", 1);
                }
                out.close();
                outMutex.lock();
                tempfiles.push_back(filename);
                outMutex.unlock();
                printf("Compelete the %d block.\n", order);
            }
            return;
        }
        // 缓冲区判满，排序，输出
        if (count >= max) {
            inMutex.unlock();
            printf("Sorting the %d block.\n", order);
            std::sort(buf.begin(), buf.begin() + count);
            std::string filename(std::string("temp") + std::string(std::to_string(order)));
            auto out = std::ofstream(filename);
            printf("Outputing the %d block.\n", order);
            for (int i = 0; i < count; i++) {
                int n = buf[i];
                out.write(std::to_string(n).c_str(), std::to_string(n).length());
                out.write("\n", 1);
            }
            out.close();
            outMutex.lock();
            tempfiles.push_back(filename);
            outMutex.unlock();
            printf("Compelete the %d block.\n", order);
            order += N;
            count = 0;
            inMutex.lock();
        }
    }
}


/**
 * 做了一些优化，包括：
 * 多线程处理第一遍分块排序，尽量利用读磁盘时CPU等待的时间
 * 手动维护一个缓冲区，用于处理读大量文件时可能出现的页缓存miss的问题
 * TODO：空缓冲区填充的问题：
 *         用多线程处理缓冲区：每个线程关心自己的<文件流，队列>对，如果队列大小半空，那么填充队列
 */
void Merge_Sort::advance_merge_sort() {
    // 打开文件
    std::ifstream in(this->srcFile);
    std::vector<std::string> tempfiles;
    std::mutex inMutex;
    std::mutex outMutex;
    // 开N个线程，读数据，排序，写数据
    std::vector<std::thread> threads(N);
    for (int i = 0; i < N; i++) {
        threads.push_back(std::thread(thread_run, std::ref(in), std::ref(inMutex), i, this->LEN, std::ref(tempfiles), std::ref(outMutex)));
    }
    for (auto& it : threads) {
        if (it.joinable()) it.join();
    }
    printf("Compelete chuncking the whole src file and sorting the temp files.\n");

    // 归并文件，优化点：手动在内存中开辟缓存。优先队列先从缓存中读，缓存不够再从磁盘读。
    printf("Start to merge the temp files...\n");
    auto out = std::ofstream(std::string("result.txt"));
    auto buffer = std::vector<std::pair<std::ifstream, std::queue<int>>>(); // 数组里面是<文件输入流, 缓冲队列>对
    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, cmp> priority_q; //用于取出归并时最大值的优先队列
    for (auto i : tempfiles) {
        buffer.push_back(std::make_pair(std::ifstream(i), std::queue<int>()));
    }
    // (修订)从多个文件中，每个文件读出一块放进缓冲区里，如果读完了，就将可用数-1
    int available = buffer.size();
    std::string temp;

    // 初始填满缓冲区
    for (auto b = buffer.begin(); b != buffer.end(); b++) {
        auto& f = b->first;     //文件流
        auto& q = b->second;    //缓冲队列
        while (q.size() <= CacheCount) {
            if (std::getline(f, temp)) {
                q.push(std::stoi(temp));
            }
            else {
                if (q.empty()) {
                    available -= 1;
                }
                break;
            }
        }
    }
    // 初始化填入优先队列
    for (auto b = buffer.begin(); b != buffer.end(); b++) {
        auto& f = b->first;     //文件流
        auto& q = b->second;    //缓冲队列
        if (!q.empty()) {
            priority_q.push(std::make_pair(q.front(), (int)(b - buffer.begin())));
            q.pop();
        }
        else {
            // 一般不会执行这里
            while (q.size() <= CacheCount) {
                if (std::getline(f, temp)) {
                    q.push(std::stoi(temp));
                }
                else {
                    if (q.empty()) {
                        available -= 1;
                    }
                    break;
                }
            }
        }
    }

    do {
        // 从优先队列中取出最小的数
        int num = priority_q.top().first;
        int pos = priority_q.top().second;

        // 写入文件 
        std::string s = std::to_string(num) + "\n";
        out.write(s.c_str(), s.length());
        priority_q.pop();

        // 在相应的文件中再读一个数，如果读不出来，就将可用数-1(废弃)
        // (修订)从相应的缓冲区队列中再读一个数，如果队列空，那么从文件流填入缓冲区。

        if (!buffer[pos].second.empty()) {
            priority_q.push(std::make_pair(buffer[pos].second.front(), pos));
            buffer[pos].second.pop();
        }
        else {
            while (buffer[pos].second.size() <= CacheCount) {
                if (std::getline(buffer[pos].first, temp)) {
                    if (std::stoi(temp) == 0) {
                        printf("a");
                    }
                    buffer[pos].second.push(std::stoi(temp));
                }
                else {
                    if (buffer[pos].second.empty()) {
                        available -= 1;
                    }
                    break;
                }
            }
            // 快速给优先队列续命，要不然就会出现优先队列的大小和available不相等，破坏不变性，结果是top()未定义。
            if (!buffer[pos].second.empty()) {
                priority_q.push(std::make_pair(buffer[pos].second.front(), pos));
                buffer[pos].second.pop();
            }
        }
        // 直到available == 0，缓冲区的所有队列都清空。
    } while (available > 0);
    // 关于优先队列与可用文件数，满足这样的不变性：
    // 如果优先队列非空，那么一定有可用的文件。
    printf("\nCompelete merge-sorting........%ld\n", priority_q.size());
}