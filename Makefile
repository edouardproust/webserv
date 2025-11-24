NAME = webserv
NAME_DEV = $(NAME)_dev

CXX = c++

CXXFLAGS := -Wall -Wextra -Werror -std=c++98

# ------- CONFIG --------

CONFIG_SRC_DIR = tests/config
CONFIG_DST_DIR = $(CONFIG_SRC_DIR)/replaced

CONFIG_FILE = webserv.config
CONFIG_FILE_42 = ubuntu_tester.config
CONFIG_FILE_WELCOME = welcome.config

CONFIG_REPL_SH = $(CONFIG_SRC_DIR)/replace-path.sh

# ------- LOG --------

LOG_DIR = log
LOG_ACCESS = $(LOG_DIR)/access.log
LOG_ERROR = $(LOG_DIR)/error.log
LOG_REDIRECTION = 1>$(LOG_ACCESS) 2>$(LOG_ERROR)

# ------- Sources -------

BASE_SRC_FILES = \
	main.cpp \
	config/Config.cpp \
	config/ServerBlock.cpp \
	config/LocationBlock.cpp \
	config/HostPortPair.cpp \
	network/Network.cpp \
	network/Socket.cpp \
	http/Request.cpp \
	http/RequestParser.cpp \
	http/Response.cpp \
	http/HttpStatus.cpp \
	router/Router.cpp \
	router/RoutingDecision.cpp \
	router/RedirectionHandler.cpp \
	static/StaticHandler.cpp \
	cgi/CgiHandler.cpp \
	utils/Const.cpp \
	utils/utils.cpp \
	utils/signal.cpp \
	utils/Log.cpp \
	utils/PrintableString.cpp \

DEV_SRC_FILES =

SRC_DIR = src

# ------- Objects / Deps -------

OBJ_DIR = build

PROD_CXXFLAGS := $(CXXFLAGS) -O2
PROD_OBJ_DIR = $(OBJ_DIR)/prod
PROD_SRCS = $(addprefix $(SRC_DIR)/, $(BASE_SRC_FILES))
PROD_OBJS = $(addprefix $(PROD_OBJ_DIR)/, $(BASE_SRC_FILES:.cpp=.o))
PROD_DEPS = $(PROD_OBJS:.o=.d)

DEV_CXXFLAGS := $(CXXFLAGS) -DDEVMODE=1
DEV_OBJ_DIR = $(OBJ_DIR)/dev
DEV_SRCS = $(addprefix $(SRC_DIR)/, $(BASE_SRC_FILES) $(DEV_SRC_FILES))
DEV_OBJS = $(addprefix $(DEV_OBJ_DIR)/, $(BASE_SRC_FILES:.cpp=.o) $(DEV_SRC_FILES:.cpp=.o))
DEV_DEPS = $(DEV_OBJS:.o=.d)

# ------- Includes -------

INC_DIR = inc
INC_FLAGS = -I$(INC_DIR)

# ------- Dependencies -------

DEPS_FLAGS = -MMD -MP

# ------- Rules -------

.PHONY: all clean fclean re dev test_prod test_dev test_42 test_welcome

all: $(NAME)

$(NAME): $(PROD_OBJS)
	$(CXX) $(PROD_OBJS) -o $@

$(NAME_DEV): $(DEV_OBJS)
	$(CXX) $(DEV_OBJS) -o $@ $(DEV_CXXFLAGS)

$(PROD_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(PROD_CXXFLAGS) $(DEPS_FLAGS) $(INC_FLAGS)

$(DEV_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(DEV_CXXFLAGS) $(DEPS_FLAGS) $(INC_FLAGS)

-include $(PROD_DEPS)
-include $(DEV_DEPS)

dev: $(NAME_DEV)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(NAME_DEV)
	rm -rf $(LOG_DIR)
	rm -rf $(CONFIG_DST_DIR)
	rm -rf $(TMP_FILES_DIR)

re: fclean all

# ------- Tests -------

test_prod: all
	@mkdir -p $(dir $(LOG_ACCESS)) $(dir $(LOG_ERROR))
	@bash $(CONFIG_REPL_SH) $(CONFIG_SRC_DIR)/$(CONFIG_FILE) $(CONFIG_DST_DIR)
	./$(NAME) $(CONFIG_DST_DIR)/$(CONFIG_FILE) $(LOG_REDIRECTION)

test_dev: dev
	@clear
	@bash $(CONFIG_REPL_SH) $(CONFIG_SRC_DIR)/$(CONFIG_FILE) $(CONFIG_DST_DIR)
	valgrind ./$(NAME_DEV) $(CONFIG_DST_DIR)/$(CONFIG_FILE)

test_42: dev
	@clear
	bash $(CONFIG_REPL_SH) $(CONFIG_SRC_DIR)/$(CONFIG_FILE_42) $(CONFIG_DST_DIR)
	valgrind ./$(NAME_DEV) $(CONFIG_DST_DIR)/$(CONFIG_FILE_42)

test_welcome: all
	@clear
	./$(NAME) $(CONFIG_FILE_WELCOME) $(LOG_REDIRECTION)
