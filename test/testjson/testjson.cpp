#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

string func1()
{
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    string res = js.dump();
    return res;
}

int main()
{
    string s = func1();
    json js2 = json::parse(s);
    vector<int>a = js2["id"];
    for (auto i : a) cout << i << '.' ;
    cout << endl;
    return 0;
}