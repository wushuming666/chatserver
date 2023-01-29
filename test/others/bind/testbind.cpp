#include <iostream>
#include <functional>
#include <cstdio>
using namespace std;
using namespace placeholders;
class Test {
public:
    void func(int a, int b, int c, int d, int e) 
    {
        printf("%d %d %d %d %d\n", a, b, c, d, e);
    }
};

int main() {
    //当参数为类内非静态成员函数时，第一个参数必须使用&符号。
    auto f = bind(&Test::func, Test(), _1, 12, _3, 5, _2);
    f(10, 6, 7); 
    f.operator()(10, 6, 7);
}
//都输出 10 12 7 5 6