EXEC=server_device
CC=gcc
OBJ=main.o   #需要增加的地方
CFLAGS=-Wall -g
LFLAGS=-lpthread -ldl -lrt -lm -lsqlite3

$(EXEC):$(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS) $(LIBS)
%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LFLAGS) 
clean:
	rm $(EXEC) *.o