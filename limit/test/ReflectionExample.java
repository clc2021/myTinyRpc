import java.lang.reflect.*;

public class ReflectionExample {
    public static void main(String[] args) throws Exception {
        // 获取类的 Class 对象
        Class<?> clazz = MyClass.class;

        // 获取类的名称
        System.out.println("Class name: " + clazz.getName()); // Class name: MyClass

        // 获取类的所有方法
        Method[] methods = clazz.getDeclaredMethods();
        System.out.println("Methods:");
        for (Method method : methods) {
            System.out.println(method.getName()); // myMethod
        }

        // 获取类的所有字段
        Field[] fields = clazz.getDeclaredFields();
        System.out.println("Fields:");
        for (Field field : fields) {
            System.out.println(field.getName()); // myField
        }

        // 调用类的方法
        MyClass obj = new MyClass();
        Method method = clazz.getMethod("myMethod");
        method.invoke(obj);
        
        // 获取字段的值
        Field field = clazz.getDeclaredField("myField");
        field.setAccessible(true); // 设置字段为可访问
        String value = (String) field.get(obj);
        System.out.println("Field value: " + value);
    }
}
