NAME				=	webserv
VPATH				=	src src/parser
SRC					=	main.cpp parser.cpp Server.cpp Connection.cpp
OBJ					=	$(addprefix ./obj/, $(SRC:%.cpp=%.o))
CPPFLAGS			=	-Wall -Wextra -Werror -std=c++17 -I include
LDFLAGS				=
CC					=	c++

FSANITIZE			=	-g -fsanitize=address

COL_GREEN			= 	\033[32m
COL_RED				=	\033[31m
COL_YELLOW			= 	\033[38;2;214;189;28m
COL_PINK			= 	\033[95m
COL_DEFAULT			= 	\033[0m

.SILENT:

all: $(NAME)

$(NAME): $(OBJ)
	echo "$(COL_YELLOW)Building $(NAME)...$(COL_DEFAULT)"
	$(CC) $(OBJ) -o $(NAME) $(LDFLAGS)
	echo "$(COL_GREEN)Successfully built $(NAME).$(COL_DEFAULT)"

./obj/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf obj
	echo "$(COL_GREEN)Object files have been removed.$(COL_DEFAULT)"

fclean: clean
	rm -f $(NAME)
	echo "$(COL_GREEN)$(NAME) has been removed.$(COL_DEFAULT)"

re: fclean all

f: LDFLAGS += $(FSANITIZE)
f: CPPFLAGS += $(FSANITIZE)
f: re

.PHONY: all clean fclean re f
