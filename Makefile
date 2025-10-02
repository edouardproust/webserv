NAME = webserv

CC = c++

FLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP

SRCS_FILES = main.cpp

SRCS_DIR = src

SRCS = $(addprefix $(SRCS_DIR)/, $(SRCS_FILES))

OBJS_DIR = $(SRCS_DIR)/_obj

OBJS = $(addprefix $(OBJS_DIR)/, $(SRCS_FILES:.cpp=.o))

DEPS = $(OBJS:.o=.d)


# ------- Rules -------

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $^ -o $@

$(OBJS_DIR)/%.o: %.cpp Makefile
	mkdir -p $(OBJS_DIR)
	$(CC) -c $< -o $@ $(FLAGS)

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all


# ------- Other -------

-include $(DEPS)

.PHONY: all clean fclean re
