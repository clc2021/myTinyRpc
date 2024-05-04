// #ifndef LIMIT_ASPECT_H
// #define LIMIT_ASPECT_H

// #include "LimitProcess.h"
// #include "LimitingRule.h"
// // #include "RateLimit.h"
// #include <thread>
// #include <memory>
// #include <unordered_map>
// #include <functional>
// #include <stdexcept> // 包含异常类的头文件

// class Method { // 方法限流中，被限流的方法 +to_do 还没做呢
// private:
//     std::function<void()> method;
//     std::string name;

// public:
//     std::string getName() {
//         return name;
//     }
// };

// /*
//   @RequestMapping("/order")
//     @LimitingStrategy(strategyName = "test",limitKey = "id",QPS = 3)
//     public void getOrder( Person p)  {
//         System.out.println("执行放行");
//     }
// */

// class LimitAspect {
// public:
//     LimitProcess limitProcess; // 处理方式

//     //thread_local std::unique_ptr<LimitingRule> threadLocal; 
//     std::unordered_map<std::thread::id, LimitingRule> threadLocal; // to_do ??

//     LimitAspect() {}

//     // 还是用模板,通过指针或引用传递对象，并根据属性名查找属性值
//     // 比如这里用appId-LoginRequest?
//     template <typename T> // 这个T可以是UserService类型，其实就是服务类型
//     findObjectAttribute(const std::string& fieldName, const T& obj) {
//         if (fieldName == "getName")
//             return obj.getName();
//         else if (fieldName == "Login")
//             return "Login";
//         else    
//             return "Register"; 
//     }

//     template <typename T>
//     auto annotationAround(const T& obj) { // 这里的T=UserService
//         bool pass = false;
//         Method method;
//         // 不使用统一限流配置，因此删除了相应逻辑
//         /// to_do + 如果顺利之后可以加上统一限流配置
//         LimitingRule rule; // 就用默认值
//         rule.setId(method.getName()); // to_do 这里不太明白是唯一标识了什么?

//         // 如果limitKey为空则直接对方法进行限流处理
//         if (rule.getLimitKey() == "") {
//             pass = limitProcess.limitHandle(rule); // 进行限流处理
//         } 
//         // 如果limitKey不空则对limitKey对应是属性进行限流处理
//         else {
//             // to_do：方法的参数名
//             // to_do: 方法的参数值
//             // to_do: java里的是RateLimit limitingStrategy = method.getAnnotation(RateLimit.class);
//             // to_do：然后limitStrategy.limitKey(), 并且使用它初始化rule
//             std::string limitKey = rule.getLimitKey(); // to_do。java里的和这个不一样
//             auto value = findObjectAttribute<T>("getName", obj); // 得到value
//             // to_do : 这里value直接用的用户名，之后可不可以完全实现java的？
//             if (value != nullptr) {
//                 // 比如用户"张三"
//                 rule.setLimitValue(static_cast<void*>(const_cast<char*>(value.c_str()))); // value
//                 pass = limitProcess.limitHandle(rule);
//             } else {
//                 std::cout << "limitKey未找到对应的属性, 请检查limitKey是否存在!本次对方法限流" << std::endl;
//                 pass = limitProcess.limitHandle(rule);
//             }

//             if (pass) {
//                 std::cout << "限流完毕，返回" << std::endl;
//                 return nullptr;
//             } else { // 没有通过
//                 auto fallBackClass = rule.getFallBackClass(); // 获取降级类
//                 if (fallBackClass == nullptr) {
//                     // to_do 这里的method到底是什么？
//                     std::cout << "方法: " << method.getName() << "执行被限流拦截, 未配置降级处理, 本次返回nullptr! " << std::endl;
//                     return nullptr; 
//                 } 

//                 // to_do 这里之后要执行得和java一样
//                 fallBackClass.handleFallback1(); 
//                 std::cout < "降级方法执行成功" << std::endl;
        
//             }
//         }

//         return nullptr;

//     }
// };
// #endif