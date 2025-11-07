#include "../RedCppLib.hpp"

using namespace Red;

int main()
{
    vec2<int> v = {0, 1};
    v += vec2<int>{2, -1};
    return v.y;
}