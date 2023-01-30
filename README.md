# chatserver
可以工作在nginx tcp负载均衡环境中的集群聊天服务器和客户端源码 基于muduo实现

## 编译方式
1. cd build
2. rm -rf *
3. cmake ..
4. make

也可以装cmake tools 一键编译

# 1、项目需求
1. 客户端新用户注册
2. 客户端用户登录
3. 添加好友和添加群组
4. 好友聊天
5. 群组聊天
6. 离线消息
7. nginx配置tcp负载均衡
8. 集群聊天系统支持客户端跨服务器通信

# 2、Json
Json是一种轻量级的数据交换格式。独立于编程语言、宜上手等特点使Json能够有效地提高网路传输效率。

接下来介绍Json的使用

```cpp
#include "json.hpp"  
using json = nlohmann::json;

#include <iostream>
#include <string>
using namespace std;

int main()
{
    json js;
    js["id"] = 1;
    js["name"] = "zhang san";
    cout << "js: " << js << endl;

    string s = js.dump();           // 将json转为string
    cout << "s: " << s << endl;

    json js2 = json::parse(s);      // 将string转为json
    cout << "js2: " << js2 << endl;

    int id = js["id"].get<int>();   // 处理json里面的int
    cout << id << endl;
    return 0;
}
```

# 3、muduo
`muduo` 是一个网络库，给用户提供了两个主要的类：
1. `TcpServer`: 用于编写服务端程序
2. `TcpClient`: 用于编写客户端程序
--------------------------------------------------------------------------------------------
`muduo` 的使用可以暂时理解为直接套板子即可。

它的搭板子流程大致如下：
```
3. 组合TcpServer对象
4. 创建eventloop事件循环对象的指针
5. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
6. 在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写的回调函数
7. 设置合适的服务端线程数量，muduo库会自己划分I/O线程和worker线程
```

-----------
下面提供一个测试代码

```cpp
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

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
    //专门处理用户的连接创建和断开
    void onConnection(const TcpConnectionPtr&conn)
    {
        if (conn->connected()) {
            cout << conn->peerAddress().toIpPort() << "->" <<
                conn->localAddress().toIpPort() << " state: online" << endl;
        } else {
            cout << conn->peerAddress().toIpPort() << "->" <<
                conn->localAddress().toIpPort() << " state: offline" << endl;
            conn->shutdown();
        }
    }

    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr&conn, //连接
                    Buffer *buffer,                //缓冲区
                    Timestamp time)  
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << "time: " << time.toString() << endl;
        string sendbuf = "来自服务端的消息: " + buf;
        conn->send(sendbuf);    //发送给客户端
    }   

    TcpServer _server; 
    EventLoop *_loop;
};

int main()
{
    EventLoop loop;     //epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}
```
编译方式

```bash
g++ -o testmuduo my_muduo_server.cpp -lmuduo_net -lmuduo_base -lpthread
```
运行结果

![在这里插入图片描述](https://img-blog.csdnimg.cn/f0b6379464e641718d86635e98fdc25b.png)

---------
分析上面的测试代码可知有几个是在写死了的板子以外的API
* `conn->connected() ` 是否连接
* `conn->peerAddress().toIpPort()`、`conn->localAddress().toIpPort()`  来源和本地的ip+端口
* `string buf = buffer->retrieveAllAsString()` 服务端接收客户端传过来的数据
* `conn->send(sendbuf)` 服务端将sendbuf发给客户端

# 4、CMake
`CMake` 相比于手写 `Makefile` 友好太多了。手写 `Makefile` 是一场噩梦。

`CMake` 使用起来就是指下编译器去哪个文件夹找文件，对于我做的这个项目来说还是挺容易的。

没有太大的难度，用的时候查下就行了，贴两个具有代表性的上来。
```bash
# 主入口
cmake_minimum_required(VERSION 3.0)
project(chat)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置最终的可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)

# 加载子目录
add_subdirectory(src)
```
```bash
# src/server
# 定义一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
# 指定可执行文件链接时需要依赖的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)
```
# 5、MySQL
MySQL 模块只用关心调用数据库的 API 实现。

有以下几个操作：初始化数据库连接、释放数据库连接资源、连接数据库、更新操作、查询操作、获取连接。

直接贴代码

```cpp
#include "db.h"
#include <muduo/base/Logging.h>

static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

//初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}

//释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr) 
        mysql_close(_conn);
}

//连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                            password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if(p != nullptr)
    {
        mysql_query(_conn, "set name gbk");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
    }
    return p;
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql << "更新失败!";
        return false;
    }
    return true;
}

// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if(mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}
```

# 6、网络模块
`chatserver.hpp` 和 `chatserver.cpp`。和`testmuduo`的代码几乎一样，主要处理连接事件和读写事件的成功接收发送。

**连接**：如果客户端断开了连接，从map表删除用户的连接信息、将用户更新为下线。
**消息**：接收所有消息后反序列化，通过解析`js["msgid"]`来获得一个业务处理器handler，再调用相应的函数。

```cpp
//chatserver.cpp
#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)
            :_server(loop, listenAddr, nameArg) ,_loop(loop)
{
    //注册连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    
    //注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    
    //设置线程数量
    _server.setThreadNum(4);
}

//启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn)
{
    // 客户端断开连接
    if (!conn->connected()) 
	{
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn,
                        Buffer* buffer,
                        Timestamp time)
{
    string buf = buffer->retrieveAllAsString();

    cout << buf << endl;
    // 数据的反序列化
    json js = json::parse(buf);
    // 目的: 完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取一个业务处理器handler 
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); //json类型转成int
    // 回调消息绑定好的事件处理器, 来执行相应的业务
    msgHandler(conn, js, time);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
}
```

# 7、业务模块
前面的数据库模块和网络模块解决了我们调用的API，接下来就是业务逻辑了！直接上代码说明下业务模块需要解决的问题

```cpp
//chatservice.hpp
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "redis.hpp"
#include "groupmodel.hpp"
#include "friendmodel.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "json.hpp"

using json = nlohmann::json;
//处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

//聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 服务器异常，业务重置方法
    void reset();
	// 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    ChatService()
    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;
    // 存储在线用户的通信连接 线程安全
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;
    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    // redis操作对象
    Redis _redis;
};

#endif
```

再处理单个业务时先解析`json`字符串得到数据，再通过这些数据进行相应的处理。需要注意的是`STL`本身是线程不安全的，所以在处理`STL`时需要加锁。

-----------
我这里具体分析下一对一聊天业务，其它的业务处理流程大同小异。

在构造函数里会对业务相关的事件处理注册回调函数
```cpp
_msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
// 还有很多，这里不一一列举
```
服务器接收到`json`（**服务端和客户端会相互约定好发送格式**）后会解析出信息。比如一对一聊天的`json`格式为：
```cpp
json js;
js["msgid"] = ONE_CHAT_MSG;
js["id"] = g_currentUser.getId();
js["name"] = g_currentUser.getName();
js["toid"] = friendid;
js["msg"] = message;
js["time"] = getCurrentTime();
string buffer = js.dump();
```
然后再根据`toid`是否在线选择及时发送消息还是存储到离线消息里面。

```cpp
// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end()) 
        {
            // toid 在线，转发消息  服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }
    // 查询toid是否在线 
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }
    // toid 不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}
```

# 8、ngnix
`ngnix` 是个负载均衡器，用于服务器集群。需要自行配置`tcp`负载均衡。

在 `root` 用户下进行如下配置：
![在这里插入图片描述](https://img-blog.csdnimg.cn/8b5c4d45a555443fb0ca4d486b7473aa.png)
```cpp
# ngnix tcp loadbalance config
stream {
	upstream MyServer {
		server 127.0.0.1:6000 weight=1 max_fails=3 fail_timeout=30s;
		server 127.0.0.1:6002 weight=1 max_fails=3 fail_timeout=30s;
	}

	server {
		proxy_connect_timeout 1s;
		#proxy_timeout 3s;
		listen 8000;
		proxy_pass MyServer;
		tcp_nodelay on;
	}
}
```
再重启（ngnix支持平滑重启，可是我没试成功）下就ok了。

效果如下
![在这里插入图片描述](https://img-blog.csdnimg.cn/a31ffddb5f0f4c0a8dea7855b9bdc7d0.png)

# 9、redis
`ngnix` 实现了集群，可是如果有两个用户登录在了不同的服务器，他们应该怎样通信呢？最好的方式就是引入中间件消息队列，解耦各个服务器，使整个系统
松耦合，提高服务器的响应能力，节省服务器的带宽资源。
![在这里插入图片描述](https://img-blog.csdnimg.cn/3ec8bd61b87744ed9439ca5f75379cc8.png)

`redis` 采用的 发布-订阅 模式，本质上是一个存储 键值对 的缓存数据库。

`redis` 的简易使用
![在这里插入图片描述](https://img-blog.csdnimg.cn/71dddeee0ed54c50a1735b1840b52904.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/8a736db25a214647944c231f754ea0d8.png)

----
`redis` 要实现的功能如下
```cpp
//redis.hpp
#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

/*
redis作为集群服务器通信的基于发布-订阅消息队列时，会遇到两个难搞的bug问题
https://blog.csdn.net/QIANGWEIYUAN/article/details/97895611
*/
class Redis
{
public:
    Redis();
    ~Redis();
    // 连接redis服务器 
    bool connect();
    // 向redis指定的通道channel发布消息
    bool publish(int channel, string message);
    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);
    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);
    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();
    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);
private:
    // hiredis同步上下文对象，负责publish消息
    redisContext *_publish_context;
    // hiredis同步上下文对象，负责subscribe消息
    redisContext *_subcribe_context;
    // 回调操作，收到订阅的消息，给service层上报
    function<void(int, string)> _notify_message_handler;
};
#endif
```

**连接redis服务器：** 发布消息和订阅消息绑定ip+端口；开个单独的线程用于监听通道上的事件。
**发布消息：** 直接调用 `redisCommand`。这个api包含三步：
```
#1 redisAppendCommand 把消息写到本地缓存
#2 redisBufferWrite 发送给服务器
#3 redisGetReply 阻塞等待消息
```
**订阅消息：** 等待消息是阻塞的，所以不要在这个函数里面阻塞等待。只进行前两步。
**取消订阅：** 和订阅的大致步骤一样
**接收订阅的消息：** 独立线程。调用`ChatService`在构造函数传过来的函数名`handleRedisSubscribeMessage`

# 10、表设计
User

| 字段名称 | 字段类型                  | 字段说明     | 约束                        |
| -------- | ------------------------- | ------------ | --------------------------- |
| id       | INT                       | 用户id       | PRIMARY KEY、AUTO_INCREMENT |
| name     | VARCHAR(50)               | 用户名       | NOT NULL, UNIQUE            |
| password | VARCHAR(50)               | 用户密码     | NOT NULL                    |
| state    | ENUM('online', 'offline') | 当前登录状态 | DEFAULT 'offline'           |



Friend

| 字段名称 | 字段类型 | 字段说明 | 约束               |
| -------- | -------- | -------- | ------------------ |
| userid   | INT      | 用户id   | NOT NULL、联合主键 |
| friendid | INT      | 好友id   | NOT NULL、联合主键 |



AllGroup

| 字段名称  | 字段类型     | 字段说明   | 约束                        |
| --------- | ------------ | ---------- | --------------------------- |
| id        | INT          | 组id       | PRIMARY KEY、AUTO_INCREMENT |
| groupname | VARCHAR(50)  | 组名称     | NOT NULL,UNIQUE             |
| groupdesc | VARCHAR(200) | 组功能描述 | DEFAULT ''                  |

​       

GroupUser

| 字段名称  | 字段类型                  | 字段说明 | 约束               |
| --------- | ------------------------- | -------- | ------------------ |
| groupid   | INT                       | 组id     | NOT NULL、联合主键 |
| userid    | INT                       | 组员id   | NOT NULL、联合主键 |
| grouprole | ENUM('creator', 'normal') | 组内角色 | DEFAULT ‘normal’   |

​     

OfflineMessage

| 字段名称 | 字段类型     | 字段说明                   | 约束     |
| -------- | ------------ | -------------------------- | -------- |
| userid   | INT          | 用户id                     | NOT NULL |
| message  | VARCHAR(500) | 离线消息（存储Json字符串） | NOT NULL |
