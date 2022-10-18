#include "../wyze/wyze.h"


wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void test_assert()
{
    WYZE_ASSERT2(0 == 1, "AABBCDD");
}


int main(int argc, char** argv) 
{
    test_assert();
    return 0;
}