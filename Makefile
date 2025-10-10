NAME = webserv

CXX = c++

CXXFLAGS := -Wall -Wextra -Werror -std=c++98

# ------- Sources -------

BASE_SRC_FILES = \
	main.cpp \
	utils/utils.cpp \
	config/Config.cpp \
	config/LocationBlock.cpp \
	config/ServerBlock.cpp \
	http/Request.cpp \
	http/RequestParser.cpp

DEV_SRC_FILES = \
	http/dev.http.cpp

SRC_DIR = src

# ------- Objects / Deps -------

PROD_CXXFLAGS := $(CXXFLAGS) -O2
PROD_OBJ_DIR = obj
PROD_SRCS = $(addprefix $(SRC_DIR)/, $(BASE_SRC_FILES))
PROD_OBJS = $(addprefix $(PROD_OBJ_DIR)/, $(BASE_SRC_FILES:.cpp=.o))
PROD_DEPS = $(PROD_OBJS:.o=.d)

DEV_CXXFLAGS := $(CXXFLAGS) -g -DDEVMODE=1 -fsanitize=address
DEV_OBJ_DIR = $(PROD_OBJ_DIR)/dev
DEV_TARGET = $(NAME)_dev
DEV_SRCS = $(addprefix $(SRC_DIR)/, $(BASE_SRC_FILES) $(DEV_SRC_FILES))
DEV_OBJS = $(addprefix $(DEV_OBJ_DIR)/, $(BASE_SRC_FILES:.cpp=.o) $(DEV_SRC_FILES:.cpp=.o))
DEV_DEPS = $(DEV_OBJS:.o=.d)

# ------- Includes -------

INC_DIR = inc
INC_FLAGS = -I$(INC_DIR)

# ------- Dependencies -------

DEPS_FLAGS = -MMD -MP

# ------- Rules -------

.PHONY: all clean fclean re dev

all: $(NAME)

$(NAME): $(PROD_OBJS)
	$(CXX) $(PROD_OBJS) -o $@

dev: $(DEV_TARGET)

$(PROD_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(PROD_CXXFLAGS) $(DEPS_FLAGS) $(INC_FLAGS)

$(DEV_TARGET): $(DEV_OBJS)
	$(CXX) $(DEV_OBJS) -o $@ $(DEV_CXXFLAGS) -fsanitize=address

$(DEV_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(DEV_CXXFLAGS) $(DEPS_FLAGS) $(INC_FLAGS)

-include $(PROD_DEPS)
-include $(DEV_DEPS)

clean:
	rm -rf $(PROD_OBJ_DIR) $(DEV_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all
