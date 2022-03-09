#include <cassert>
#include <string>

using namespace std;

#include "src/utils/util.hpp"

void test_util()
{
    string path = "./dwadawdafafaas";
    UtilFile::dir_remove(path);
    assert(UtilFile::dir_create(path));
    assert(UtilFile::dir_exists(path));
    assert(UtilFile::dir_remove(path));
    assert(!UtilFile::dir_exists(path));
    cout << "success" << endl;
}

int main()
{
    test_util();
}