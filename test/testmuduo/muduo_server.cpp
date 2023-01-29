/*
muduo网络库给用户提供了两个主要的类
TcpServer: 用于编写服务端程序
TcpClient: 用于编写客户端程序

epoll + 线程池
好处：能够把网络I/O的代码和业务代码区分开
业务：用户连接和断开 + 用户的可读写事件
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;
/*
基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建eventloop事件循环对象的指针
3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4. 在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写的回调函数
5. 设置合适的服务端线程数量，muduo库会自己划分I/O线程和worker线程
*/
class ChatServer
{
public:
    ChatServer(EventLoop* loop,                 //事件循环
            const InetAddress& listenAddr,      //IP + Port
            const string& nameArg)              //服务器名字
            :_server(loop, listenAddr, nameArg)
            ,_loop(loop)
    {
        //给服务器注册用户连接的创建和断开回调
        //function<void(const TcpConnectionPtr&)> func1 = bind(&ChatServer::onConnection, this, _1);
        //_server.setConnectionCallback(func1);
        //bind绑定后的类型是一个function，把private里面的函数赋值给了另一个class里的函数变量，可以在另一个class里访问我的private函数。实现了网络逻辑和业务逻辑分开
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        
        //给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        
        //设置服务端的线程数量 1个I/O线程 3个worker线程
        _server.setThreadNum(4);
    }

    //开启事件循环
    void start()
    {
        _server.start();
    }
private:
    //专门处理用户的连接创建和断开 epoll kistenfd accept

    //typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    //typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
    void onConnection(const TcpConnectionPtr&conn) 
    {
        if (conn->connected()) {
            cout << conn->peerAddress().toIpPort() << "->" <<
                conn->localAddress().toIpPort() << " state: online" << endl;
        } else {
            cout << conn->peerAddress().toIpPort() << "->" <<
                conn->localAddress().toIpPort() << " state: offline" << endl;
            conn->shutdown(); //close(fd)
            // _loop->quit();
        }
    }

    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr&conn, //连接
                    Buffer *buffer,                //缓冲区
                    Timestamp time)             //接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << " time:" << time.toString() << endl;
        conn->send(buf);
    }
    TcpServer _server; 
    EventLoop *_loop;
};

int main()
{
    EventLoop loop; //epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); //listenfd 通过 epoll_ctr 添加到epoll上
    loop.loop();    //epoll_wait  以阻塞方式等待新用户连接，已连接用户的读写事件等

    return 0;
}