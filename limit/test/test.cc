#include <iostream>
#include <string>
#include <map>

class Request {
public:
    std::string getAppId() const {
        return appId;
    }

    void setAppId(const std::string& appId) {
        this->appId = appId;
    }

private:
    std::string appId;
};

// 函数通过引用传递对象，并根据属性名查找属性值
std::string findObjectAttribute(const std::string& fieldName, const Request& obj) {
    // 使用map模拟对象的属性
    std::map<std::string, std::string> attributes;

    // 假设对象的属性已经在属性映射中初始化
    // 这里仅作示例，实际应用中可能需要根据实际情况初始化属性映射
    attributes["appId"] = obj.getAppId();  // 这里假设属性是字符串类型

    // 查找属性值并返回
    auto it = attributes.find(fieldName);
    if (it != attributes.end()) {
        return it->second;
    } else {
        // 如果属性不存在，返回空字符串
        return "";
    }
}

int main() {
    Request request;
    request.setAppId("app123");

    // 调用 findObjectAttribute() 函数获取属性值
    std::string appId = findObjectAttribute("appId", request);
    std::cout << "App Id: " << appId << std::endl;

    return 0;
}
