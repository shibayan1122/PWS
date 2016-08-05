CC      = gcc

#
# Debug-Out Option
#
# Debug-Out (stdio & logfile)
CFLAGS  = -O2 -Wall -I. -I/usr/include -D DEBUG_LOGOUT_STDIO -D DEBUG_LOGOUT_FILE
# Debug-Out (stdio)
#CFLAGS  = -O2 -Wall -I. -I/usr/include -D DEBUG_LOGOUT_STDIO
# Debug-Out (logfile)
#CFLAGS  = -O2 -Wall -I. -I/usr/include -D DEBUG_LOGOUT_FILE
# No Debug-Out
#CFLAGS  = -O2 -Wall -I. -I/usr/include

DEST    = /pws/bin
LDFLAGS = -L/usr/lib -lm
LIBS    = -O2 -lpthread -lwiringPi
OBJS    = pws_manager.o pws_gpio.o pws_osc.o pws_btn.o pws_led.o
PROGRAM = pws_manager

.SUFFIXES:	.c .o

all:		$(PROGRAM)

$(PROGRAM):	$(OBJS)
			$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $(PROGRAM)
.c.o:
			$(CC) $(CFLAGS) -c $<

clean:;		rm -f *.o *~ $(PROGRAM)
			rm -f $(DEST)/$(PROGRAM)

install:	$(PROGRAM)
			sudo mkdir -p $(DEST)
			install -s $(PROGRAM) $(DEST)
