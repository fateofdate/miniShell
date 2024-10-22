# 指定编译器
CXX = g++
CXXFLAGS = -Iinclude -std=c++11 -Wall

# 源文件和目标文件
SRCS = MyBash.cpp
OBJS = $(SRCS:.cpp=.o)

# 可执行文件名称
TARGET = MyBash

# 默认目标
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^

# 编译源文件为目标文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标
.PHONY: all clean
