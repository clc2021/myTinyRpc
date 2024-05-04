package com.rpc.easy_rpc_govern.limit.aspect;

import com.rpc.easy_rpc_govern.limit.LimitProcess;
import com.rpc.easy_rpc_govern.limit.entity.LimitingRule;
import com.rpc.easy_rpc_govern.limit.limitAnnotation.RateLimit;
import lombok.Data;
import lombok.extern.slf4j.Slf4j;

import org.aspectj.lang.ProceedingJoinPoint;
import org.aspectj.lang.annotation.Around;
import org.aspectj.lang.annotation.Aspect;
import org.aspectj.lang.annotation.Pointcut;
import org.aspectj.lang.reflect.CodeSignature;
import org.aspectj.lang.reflect.MethodSignature;
import org.springframework.stereotype.Service;
import javax.annotation.Resource;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;


@Service
@Slf4j
@Aspect
@Data
public class LimitAspect {
    @Resource
    LimitProcess limitProcess;

    public ThreadLocal<LimitingRule> threadLocal = new ThreadLocal<>();

    @Pointcut("@annotation(com.rpc.easy_rpc_govern.limit.limitAnnotation.RateLimit)")
    private void annotationPointCut() {
    }

    @Around("annotationPointCut()")
    public Object annotationAround(ProceedingJoinPoint jp) {
        boolean pass = false;
        Method method = ((MethodSignature) jp.getSignature()).getMethod();
        RateLimit limitingStrategy = method.getAnnotation(RateLimit.class);
        
        // 执行方法配置限流
        LimitingRule rule = new LimitingRule(limitingStrategy);
        rule.setId(method.getName());
        // 如果limitKey为空则直接对方法进行限流处理
        if(rule.getLimitKey().isEmpty()){
            pass = limitProcess.limitHandle(rule);
        }
        // 如果limitKey不为空则对limitKey对应的属性进行限流处理
        else {
            String[] parameterNames = ((CodeSignature) jp.getSignature()).getParameterNames();
            Object[] args = jp.getArgs();
            String limitKeyName = limitingStrategy.limitKey();
            Object value = findObjectAttribute(parameterNames,args,limitKeyName);
            if(value != null){
                rule.setLimitValue(value);
                pass = limitProcess.limitHandle(rule);
            }
            else {
                log.error("limitKey未找到对应的属性，请检查limitKey是否存在！本次对方法限流");
                pass = limitProcess.limitHandle(rule);
            }
        }

        if(pass){
            try {
                return jp.proceed();
            } catch (Throwable throwable) {
                log.error("方法执行失败！错误信息：{}",throwable.getMessage());
            }
        }else {
            Class<?> fallBackClass = limitingStrategy.fallBack();
            if(fallBackClass == void.class){
                log.error("方法:{} 执行被限流拦截，未配置降级处理，本次返回null！",method.getName());
                return null;
            }
            try {
                Method declaredMethod = fallBackClass.getDeclaredMethod(method.getName(), method.getParameterTypes());
                Object obj = fallBackClass.newInstance();
                return declaredMethod.invoke(obj,jp.getArgs());
            } catch (NoSuchMethodException e) {
                log.error("降级方法未找到！请确认方法名：{}，参数类型：{}，是否与被拦截方法一致",method.getName(),method.getParameterTypes());
            } catch (InstantiationException | IllegalAccessException | InvocationTargetException e) {
                log.error("降级方法执行失败！错误信息：{}",e.getMessage());
            }
        }
        return null;

        ////////////////////////////////////// 方法限流配置 结束 /////////////////////////////////////
    }
    // 递归调用获取指定参数名中的属性值
    private Object findObjectAttribute(String[] parameterNames,Object[] args, String limitKeyName) {
        // 如果parameterNames[i]的类型是基本数据类型或者String类则直接获取值
        for (int i = 0; i < parameterNames.length; i++) {
            if(args[i] != null && (args[i].getClass().isPrimitive() || args[i].getClass().getName().contains("java.lang"))){
                if(parameterNames[i].equals(limitKeyName)){
                    return args[i];
                }
            }
            // 如果parameterNames[i]的类型是对象则需要递归遍历遍历对象的属性
            else if(args[i] != null){
                Field[] declaredFields = args[i].getClass().getDeclaredFields();
                String[] attrsName = new String[declaredFields.length];
                Object[] attrsValue = new Object[declaredFields.length];
                for (int j = 0; j < declaredFields.length; j++) {
                    attrsName[j] = declaredFields[j].getName();
                    attrsValue[j] = getFieldValueByName(declaredFields[j].getName(), args[i]);
                }
                Object value = findObjectAttribute(attrsName,attrsValue,limitKeyName);
                if(value != null){
                    return value;
                }
            }
        }
        return null;
    }
    // 通过get方法获取属性值
    private  Object getFieldValueByName(String fieldName, Object o) {
        try {
            String firstLetter = fieldName.substring(0, 1).toUpperCase();
            String getter = "get" + firstLetter + fieldName.substring(1);
            Method method = o.getClass().getMethod(getter, new Class[] {});
            return method.invoke(o);
        } catch (Exception e) {
            return null;
        }
    }

}
