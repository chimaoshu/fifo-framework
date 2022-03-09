#include <iostream>

#include "src/config/ConfigReader.h"

using namespace std;

void test()
{
    cout << config::get("a") << endl;
    cout << config::get("b") << endl;
    cout << config::get("c") << endl;
    cout << config::get("d") << endl;
}

int main()
{
    test();
}