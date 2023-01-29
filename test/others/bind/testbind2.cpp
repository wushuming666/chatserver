/*
一个类，构造函数里有bind，绑定的private的函数，测试结果。
*/
#include <iostream>
#include <cstdio>
#include <functional>

using namespace std;
using namespace placeholders;

class Test2
{
private:
    int _data1, _data2;
public:
    Test2(int data1, int data2)
    :_data1(data1)
    ,_data2(data2)
    {
        cout << "Test2 构造函数 _data*的值：" << _data1 << ' ' << _data2 << endl;
    }

    void callBack(int x)
    {
        cout << "调用callBack" << endl;
    }
};

class Test
{
private:
    Test2 _test;
    //模拟处理
    void solve(int x){}
public:
    Test(int a, int b)
    :_test(a, b)
    {
        _test.callBack(std::bind(&Test::solve, this, _1));
    }
};


int main()
{

}