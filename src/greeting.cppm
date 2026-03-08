module;

// 这里是 "global module fragment"，用来 #include 传统头文件
// 只有还没提供 module 的库才需要在这里 include
#include <iostream>
#include <string>

// 声明这是一个 module，名字叫 greeting
export module greeting;

// export 的函数/类/变量 对外可见（类似 public header 中的声明）
// 没有 export 的则是 module 内部私有的（类似 .cpp 中的 static / 匿名 namespace）

export void greet(const std::string& name) {
  std::cout << "Hello, " << name << "!\n";
}
