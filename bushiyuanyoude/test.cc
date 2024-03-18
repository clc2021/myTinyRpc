#include <iostream>
#include <google/protobuf/descriptor.h>
#include "message.pb.h"  // 包含由编译器生成的头文件

int main() {
  // 创建一个描述符池
  google::protobuf::DescriptorPool descriptor_pool;
  
  // 使用描述符池加载描述符文件
  const google::protobuf::FileDescriptor* file_descriptor = 
      descriptor_pool.BuildFile("message.pb");
  
  if (file_descriptor != nullptr) {
    // 获取消息类型的描述符
    const google::protobuf::Descriptor* descriptor = 
        file_descriptor->FindMessageTypeByName("Person");
    
    if (descriptor != nullptr) {
      std::cout << "Message Name: " << descriptor->name() << std::endl;
      
      // 遍历字段描述符
      for (int i = 0; i < descriptor->field_count(); ++i) {
        const google::protobuf::FieldDescriptor* field_descriptor = 
            descriptor->field(i);
        std::cout << "Field Name: " << field_descriptor->name() 
                  << ", Field Number: " << field_descriptor->number() << std::endl;
      }
    } else {
      std::cerr << "Message descriptor not found!" << std::endl;
    }
  } else {
    std::cerr << "File descriptor not found!" << std::endl;
  }

  return 0;
}
