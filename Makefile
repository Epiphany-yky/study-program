# 编译器配置
CXX = g++
# 编译选项：-Wall显示所有警告、-g生成调试信息、-std=c++11指定C++11标准
CXXFLAGS = -Wall -g -std=c++11 -pthread

# 目录配置
SRC_DIR = src
BUILD_DIR = build
# 目标可执行文件名
TARGET = $(BUILD_DIR)/echo_server

# 源文件列表（当前只有main.cpp，后续加thread_pool.cpp直接在这里加）
SRCS = $(SRC_DIR)/main.cpp $(SRC_DIR)/thread_pool.cpp
# 目标文件（.cpp替换为.o）
OBJS = $(SRCS:.cpp=.o)

# 默认目标：编译可执行文件
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# 编译.cpp为.o文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理所有编译产物
clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf $(BUILD_DIR)/*.o

# 伪目标，避免和同名文件冲突
.PHONY: all clean