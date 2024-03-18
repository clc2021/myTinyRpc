#include "ZookeeperUtil.h"
#include "MprpcApplication.h"

#include <semaphore.h>
#include <iostream>

// 全局的watcher观察器   zkserver给zkclient的通知
// global_watcher(zookeeper客户端句柄, 
// 触发watcher的事件类型, eg.会话事件 节点事件	这里就是会话事件
// state连接状态,  eg.连接成功 连接断开 	这里就是在连接的状态
// path路径,
// Watcher的上下文，可以是用户自定义的数据，用于在回调函数中传递额外的信息)
void global_watcher(zhandle_t *zh, int type,
                   int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
	{
		if (state == ZOO_CONNECTED_STATE)  // zkclient和zkserver连接成功
		{// 可能有多个线程或进程需要等待zookeeper客户端与服务器建立连接后，
		 // 才能继续执行一些需要连接的操作。
			sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);  // 通知其他线程或者进程
		}
	}
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源  MySQL_Conn
    }
}

// 连接zkserver
void ZkClient::Start() // == zookeeper_init()连接节点。
{
    // 加载zookeeperIP地址和端口号
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeper_ip"); // 127.0.0.1
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeper_port"); // 2181
    std::string connstr = host + ":" + port; // connstr="127.0.0.1:2181"

	// zookeeper_mt：多线程版本；zookeeper的API客户端程序提供了三个线程；API调用线程；网络I/O线程  pthread_create  poll；watcher回调线程 pthread_create
	// zookeeper_init() 是客户端用来连接zookeeper服务端的函数
	// 具体来说：127.0.0.1:2181，watch函数, 会话过期时间为30秒，nullptr, nullptr, 0
	/*
	第一个参数 connstr.c_str() 是一个 C 风格的字符串，表示连接到 ZooKeeper 服务器的地址。
	在 ZooKeeper 中，连接字符串通常采用以下格式：
	<host1>:<port1>,<host2>:<port2>,<host3>:<port3>
	其中 <host> 是 ZooKeeper 服务器的主机名或 IP 地址，<port> 是 ZooKeeper 服务器的端口号。
	多个服务器地址用逗号分隔，这样可以提高 ZooKeeper 服务的可用性和容错性。
	因此，connstr.c_str() 作为第一个参数传递给 zookeeper_init 函数，表示要连接的 ZooKeeper 服务器的地址。
	在这个例子中，它可能是一个类似 "127.0.0.1:2181" 的字符串，表示连接到本地主机的 2181 端口的 ZooKeeper 服务器。

	问：一个zookeeper服务器是一个节点吗？
	答：在 ZooKeeper 的术语中，一个 ZooKeeper 服务器通常称为一个 "节点"，但在分布式系统的上下文中，
	"节点" 这个词可能会有不同的含义。
	在 ZooKeeper 中，一个 ZooKeeper 服务器通常称为一个 "ZooKeeper 节点" 或者简称为 "节点"，
	它是 ZooKeeper 集群中的一个成员。ZooKeeper 集群由多个 ZooKeeper 服务器节点组成，这些节点协同工作
	来提供 ZooKeeper 服务。每个 ZooKeeper 节点都负责存储数据、处理客户端请求，并与其他节点协调数据一致性。
	在分布式系统中，"节点" 这个词还可能用于指代网络中的计算机或服务器。这些节点可以承载不同的角色和服务，
	例如数据库服务器、应用服务器、消息队列服务器等。在这种情况下，一个 ZooKeeper 服务器也可以被称为一个节点，
	但是在不同的上下文中可能会有不同的含义，需要根据具体的语境来理解。
	*/
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (nullptr == m_zhandle) 
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem); 

    sem_wait(&sem);
    std::cout << "zookeeper_init success!" << std::endl;
}

// path-创建的节点的路径 data-要存储在节点中的数据 datalen-数据的长度 state-节点的状态
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // 表示path的znode节点不存在
	{
		// 创建指定path的znode节点了 ZOO_OPEN_ACL_UNSAFE允许任何客户端访问
		flag = zoo_create(m_zhandle, path, data, datalen,
			&ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
		if (flag == ZOK)
		{
			std::cout << "znode create success... path:" << path << std::endl;
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			std::cout << "znode create error... path:" << path << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
	int bufferlen = sizeof(buffer);
	int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
	if (flag != ZOK)
	{
		std::cout << "get znode error... path:" << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}