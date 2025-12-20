function fibStack(n) {
    let result = 0;
    let stack = [n];  // 初始化栈
    let top = 0;      // 栈顶指针
    
    while (top >= 0) {
        let temp = stack[top];
        top--;
        
        // 基本情况：fib(0)=0, fib(1)=1
        if (temp <= 1) {
            result += temp;
        } else {
            // 模拟递归调用：push fib(n-1) 和 fib(n-2)
            top++;
            if (top >= stack.length) {
                stack.push(0);
            }
            stack[top] = temp - 1;
            
            top++;
            if (top >= stack.length) {
                stack.push(0);
            }
            stack[top] = temp - 2;
        }
    }
    
    return result;
}

// 测试
console.time('fibStack');
console.log(`fib(30) = ${fibStack(30)}`);
console.timeEnd('fibStack');