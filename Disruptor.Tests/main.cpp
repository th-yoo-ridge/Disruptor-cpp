
#include <gmock/gmock.h>

#if _MSC_VER
# pragma warning (disable: 4231) // nonstandard extension used : 'extern' before template explicit instantiation
#endif

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

