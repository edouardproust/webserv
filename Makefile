NAME = webserv
NAME_DEV = $(NAME)_dev
NAME_BONUS = $(NAME)_bonus

CXX = c++

CXXFLAGS := -Wall -Wextra -Werror -std=c++98

MANDATORY_DIR = mandatory
BONUS_DIR = bonus

# ------- CONFIG --------

MANDATORY_CONFIG_SRC_DIR = $(MANDATORY_DIR)/tests/config
BONUS_CONFIG_SRC_DIR = $(BONUS_DIR)/tests/config

MANDATORY_CONFIG_DST_DIR = $(MANDATORY_DIR)/tests/config/replaced
BONUS_CONFIG_DST_DIR = $(BONUS_DIR)/tests/config/replaced

CONFIG_FILE = webserv.config
CONFIG_FILE_42 = ubuntu_tester.config
CONFIG_FILE_WELCOME = welcome.config

CONFIG_REPL_SH = replace-path.sh

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

BONUS_SRC_FILES = \
	$(BASE_SRC_FILES) \

SRC_DIR = src

# ------- Objects / Deps -------

OBJ_DIR = build

# MANDATORY
PROD_CXXFLAGS := $(CXXFLAGS) -O2
PROD_OBJ_DIR = $(OBJ_DIR)/prod
PROD_SRCS = $(addprefix $(MANDATORY_DIR)/src/, $(BASE_SRC_FILES))
PROD_OBJS = $(addprefix $(PROD_OBJ_DIR)/, $(BASE_SRC_FILES:.cpp=.o))
PROD_DEPS = $(PROD_OBJS:.o=.d)

DEV_CXXFLAGS := $(CXXFLAGS) -DDEVMODE=1
DEV_OBJ_DIR = $(OBJ_DIR)/dev
DEV_SRCS = $(addprefix $(MANDATORY_DIR)/src/, $(BASE_SRC_FILES) $(DEV_SRC_FILES))
DEV_OBJS = $(addprefix $(DEV_OBJ_DIR)/, $(BASE_SRC_FILES:.cpp=.o) $(DEV_SRC_FILES:.cpp=.o))
DEV_DEPS = $(DEV_OBJS:.o=.d)

# BONUS
BONUS_CXXFLAGS := $(CXXFLAGS) -DDEVMODE=1 -DBONUS=1
BONUS_OBJ_DIR = $(OBJ_DIR)/bonus
BONUS_SRCS = $(addprefix $(BONUS_DIR)/src/, $(BONUS_SRC_FILES))
BONUS_OBJS = $(addprefix $(BONUS_OBJ_DIR)/, $(BONUS_SRC_FILES:.cpp=.o))
BONUS_DEPS = $(BONUS_OBJS:.o=.d)

# ------- Includes -------

MANDATORY_INC_DIR = $(MANDATORY_DIR)/inc
BONUS_INC_DIR = $(BONUS_DIR)/inc

MANDATORY_INC_FLAGS = -I$(MANDATORY_INC_DIR)
BONUS_INC_FLAGS = -I$(BONUS_INC_DIR) -I$(MANDATORY_INC_DIR)

# ------- Dependencies -------

DEPS_FLAGS = -MMD -MP

# ------- Rules -------

.PHONY: all clean fclean re dev bonus test_prod test_dev test_42 test_welcome test_bonus

all: $(NAME)

$(NAME): $(PROD_OBJS)
	$(CXX) $(PROD_OBJS) -o $@

dev: $(NAME_DEV)

$(PROD_OBJ_DIR)/%.o: $(MANDATORY_DIR)/src/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(PROD_CXXFLAGS) $(DEPS_FLAGS) $(MANDATORY_INC_FLAGS)

$(NAME_DEV): $(DEV_OBJS)
	$(CXX) $(DEV_OBJS) -o $@ $(DEV_CXXFLAGS)

$(DEV_OBJ_DIR)/%.o: $(MANDATORY_DIR)/src/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(DEV_CXXFLAGS) $(DEPS_FLAGS) $(MANDATORY_INC_FLAGS)

bonus: $(NAME_BONUS)

$(NAME_BONUS): $(BONUS_OBJS)
	$(CXX) $(BONUS_OBJS) -o $@

$(BONUS_OBJ_DIR)/%.o: $(BONUS_DIR)/src/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) -c $< -o $@ $(BONUS_CXXFLAGS) $(DEPS_FLAGS) $(BONUS_INC_FLAGS)

-include $(PROD_DEPS)
-include $(DEV_DEPS)
-include $(BONUS_DEPS)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(NAME_DEV) $(NAME_BONUS)
	rm -rf $(LOG_DIR)
	rm -rf $(MANDATORY_CONFIG_DST_DIR) $(BONUS_CONFIG_DST_DIR)

re: fclean all

# ------- Tests -------

test_prod: all
	@mkdir -p $(dir $(LOG_ACCESS)) $(dir $(LOG_ERROR)) $(MANDATORY_CONFIG_DST_DIR)
	@bash $(MANDATORY_CONFIG_SRC_DIR)/$(CONFIG_REPL_SH) $(MANDATORY_CONFIG_SRC_DIR)/$(CONFIG_FILE) $(MANDATORY_CONFIG_DST_DIR)
	./$(NAME) $(MANDATORY_CONFIG_DST_DIR)/$(CONFIG_FILE) $(LOG_REDIRECTION)

test_dev: dev
	@clear
	@mkdir -p $(MANDATORY_CONFIG_DST_DIR)
	@bash $(MANDATORY_CONFIG_SRC_DIR)/$(CONFIG_REPL_SH) $(MANDATORY_CONFIG_SRC_DIR)/$(CONFIG_FILE) $(MANDATORY_CONFIG_DST_DIR)
	valgrind ./$(NAME_DEV) $(MANDATORY_CONFIG_DST_DIR)/$(CONFIG_FILE)

test_42: dev
	@clear
	@mkdir -p $(MANDATORY_CONFIG_DST_DIR)
	@bash $(MANDATORY_CONFIG_SRC_DIR)/$(CONFIG_REPL_SH) $(MANDATORY_CONFIG_SRC_DIR)/$(CONFIG_FILE_42) $(MANDATORY_CONFIG_DST_DIR)
	valgrind ./$(NAME_DEV) $(MANDATORY_CONFIG_DST_DIR)/$(CONFIG_FILE_42)

test_welcome: all
	@clear
	./$(NAME) $(CONFIG_FILE_WELCOME) $(LOG_REDIRECTION)

test_bonus: bonus
	@clear
	@mkdir -p $(BONUS_CONFIG_DST_DIR)
	@bash $(BONUS_CONFIG_SRC_DIR)/$(CONFIG_REPL_SH) $(BONUS_CONFIG_SRC_DIR)/$(CONFIG_FILE) $(BONUS_CONFIG_DST_DIR)
	valgrind ./$(NAME_BONUS) $(BONUS_CONFIG_DST_DIR)/$(CONFIG_FILE)