NAME = ircserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98


SRCS =	src/main.cpp src/Server.cpp src/Client.cpp src/Channel.cpp src/Message.cpp \
		src/ServerClient.cpp src/ServerDispatch.cpp 								\
		src/commands/Registration.cpp src/commands/Ping.cpp src/commands/Join.cpp



OBJDIR = obj

OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY:	all	clean	fclean	re
