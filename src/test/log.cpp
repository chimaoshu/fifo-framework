#include "src/log/Log.hpp"

#include <cassert>

using namespace std;

void test()
{
    Log::debug("123");
    Log::debug("123");
    Log::debug("123");
    Log::debug("123");
    Log::debug("123");
    Log::debug("123");
    Log::error("789");
    Log::info("456");
    Log::warn("102");
}

int main()
{
    test();
}