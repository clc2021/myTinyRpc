#include <iostream>
#include <functional>
#include <string>

using namespace std;

// 定义一个注解结构体
struct RateLimit {
    int QPS;
    string limitKey;
};

// 定义一个切面类，用于实现限流逻辑
class LimitAspect {
public:
    void annotationAround(const std::function<void()>& method) {
        // 在这里实现限流逻辑
        // 这里简单地模拟一下，每隔一段时间进行一次限流
        static bool shouldLimit = false;
        shouldLimit = !shouldLimit;
        if (shouldLimit) {
            cout << "方法被限流，请求被拒绝！" << endl;
            return;
        }

        // 如果不需要限流，则调用原始方法
        method();
    }
};

class UserService {
private:
    LimitAspect aspect; // 切面对象

public:
    void Login(const std::string& name, const std::string& pwd) {
        // 这里放置 Login 方法的业务逻辑
        cout << "在服务端: Login" << endl;
        cout << "name:" << name << " pwd:" << pwd << endl;
    }

    void Register(uint32_t id, std::string name, std::string pwd) {
        // 这里放置 Register 方法的业务逻辑
        cout << "在服务端: Register" << endl;
        cout << "id" << id << "name:" << name << " pwd:" << pwd << endl;
    }
};

int main() {
    UserService userService;
    LimitAspect aspect;

    // 创建std::function对象，并绑定到Login方法上
    std::function<void()> loginMethod = std::bind(&UserService::Login, &userService, "username", "password");

    // 应用切面，对Login方法进行限流
    aspect.annotationAround(loginMethod);

    return 0;
}
