#include "src/ThreadPool/ThreadPool.hpp"

#include <iostream>

using namespace std;

int add(int x, int y) { return x + y; }
void test()
{
    int num = 3;
    ThreadPool pool(num);

    // 提交任务
    vector<future<int>> v;
    for (int i = 0; i < num; i++)
    {
        auto f = pool.submit(add, 1, i);
        v.push_back(std::move(f));
    }

    // join
    for (int i = 0; i < num; i++)
    {
        v[i].wait();
        cout << v[i].get() << endl;
    }

    pool.shutdown();
}

int main()
{
    test();
}