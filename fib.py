def fib_stack(n):
    result = 0
    stack = [n]  # 初始化栈
    top = 0      # 栈顶指针
    
    while top >= 0:
        temp = stack[top]
        top -= 1
        
        # 基本情况：fib(0)=0, fib(1)=1
        if temp <= 1:
            result += temp
        else:
            # 模拟递归调用：push fib(n-1) 和 fib(n-2)
            top += 1
            if top >= len(stack):
                stack.append(0)
            stack[top] = temp - 1
            
            top += 1
            if top >= len(stack):
                stack.append(0)
            stack[top] = temp - 2
    
    return result

def fib(n):
    if n <= 1:
        return n
    return fib(n-1) + fib(n-2)

# 测试
import time
start = time.time()
print(f"fib(30) = {fib(30)}")
end = time.time()
print(f"Time: {(end-start)*1000:.2f}ms")

start = time.time()
print(f"fib_stack(30) = {fib_stack(30)}")
end = time.time()
print(f"Time: {(end-start)*1000:.2f}ms")