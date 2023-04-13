EXEC=air_conditioning
CC=gcc
OBJ=main.o   #需要增加的地方
CFLAGS=-Wall -g
LFLAGS=-lpthread -ldl -lrt -lm

$(EXEC):$(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)
%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LFLAGS)
clean:
	rm $(EXEC) *.o