CC 					= g++
CFLAGS 				= -std=c++17 -O2
CPPFLAGS 			= -I $(VULKAN_SDK_DIR)/include -I $(GLM_DIR)/include -I$(GLFW_DIR)/include
LDFLAGS				= -L $(GLFW_DIR)/lib -L $(VULKAN_SDK_DIR)/lib -Wl, -rpath $(VULKAN_SDK_DIR)/lib
LDLIBS 				= -lglfw -lvulkan -ldl -lpthread -lglslang -lglslang-default-resource-limits -lSPIRV-Tools -lSPIRV-Tools-opt
MJ					= -MJ $(patsubst $(BUILD_DIR)/$(OBJ_DIR)/%.o, $(BUILD_DIR)/$(JSON_DIR)/%.part.json, $@)

VULKAN_SDK_DIR		:= $(shell echo $$HOME)/VulkanSDK/1.4.304.0/macOS
GLFW_DIR			:= $(shell brew --prefix glfw)
GLM_DIR				:= $(shell brew --prefix glm)
BUILD_DIR			:= build
OBJ_DIR				:= obj
JSON_DIR			:= json

SRCS				:= main.cpp
OBJS				:= $(patsubst %.cpp, $(BUILD_DIR)/$(OBJ_DIR)/%.o, $(SRCS))
VERTS				:= simple.vert
FRAGS				:= simple.frag
SPV 				:= $(patsubst %.vert, %_vs.spv, $(VERTS)) $(patsubst %.frag, %_fs.spv, $(FRAGS))

JSON				:= compile_commands.json
NAME				:= a.out

$(NAME): $(SPV) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(OBJS) -o $@

$(BUILD_DIR)/$(OBJ_DIR)/%.o: %.cpp | dir_guard
		$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@ $(MJ) 

%_fs.spv: %.frag
	glslc $< -o $@

%_vs.spv: %.vert
	glslc $< -o $@

test: $(NAME)
	./$(NAME)

clean:
	rm -f $(SPV)
	rm -rf $(BUILD_DIR)
	rm -f $(NAME)

json:
		$(MAKE) --always-make --keep-going $(NAME)
		(echo '[' && find . -name "*.part.json" | xargs cat && echo ']') > $(JSON)

dir_guard:
		mkdir -p $(BUILD_DIR)/$(OBJ_DIR) $(BUILD_DIR)/$(JSON_DIR)

.PHONY: test clean
