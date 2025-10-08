NAME = webserv
PARSER_TEST = parse_tester

CXX = c++

CXX_FLAGS = -Wall -Wextra -Werror -std=c++98

# ------- Sources -------

SRC_FILES = parser_debug.cpp \
	utils/utils.cpp \
	config/Config.cpp \
	config/LocationBlock.cpp \
	config/ServerBlock.cpp \
	http/Request.cpp \
	http/RequestParser.cpp \

SRC_DIR = src

SRCS = $(addprefix $(SRC_DIR)/, $(SRC_FILES))

OBJ_DIR = obj

OBJS = $(addprefix $(OBJ_DIR)/, $(SRC_FILES:.cpp=.o))

# ------- Includes -------

INC_DIR = inc

INC_FLAGS = -I $(INC_DIR)

# ------- Deps -------

DEPS_FLAGS = -MMD

DEPS = $(OBJS:.o=.d)

# ------- Rules -------

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(CXX_FLAGS) $(DEPS_FLAGS) $(INC_FLAGS)

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all
