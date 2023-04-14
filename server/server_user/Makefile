EXEC = server_user
CC = gcc
OBJ = main.o user_info.o user_login.o user_manager.o user_register.o  #需要增加的地方
CFLAGS = -Wall -g
LFLAGS = -lpthread -ldl -lrt -lm -lsqlite3

$(EXEC):$(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)
%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LFLAGS)
clean:
	rm $(EXEC) *.o
