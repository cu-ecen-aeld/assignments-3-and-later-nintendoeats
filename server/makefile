
CC ?= $(CROSS_COMPILE)gcc

SRCS =\
aesdsocket.c\
connectionlist.c\
timestamper.c\
connectionHandlerThread.c\
connectionReceiverThread.c\
localsocket.c


OBJS = $(SRCS:.c=.o)

all: aesdsocket

aesdsocket: $(OBJS)
	$(CC) $(OBJS) -o aesdsocket -lpthread -lrt -Wl,--hash-style=gnu

$(TARGET).o: $(TARGET).c
	$(CC) -c $(TARGET).c -o $(TARGET).o 

clean:
	rm -rf aesdsocket
	rm -rf *.o
	
# DO NOT DELETE
