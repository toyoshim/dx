#DEBUG=1
ifeq ($(DEBUG), 1)
	TARGET	= dx_debug
	WORK	= _Debug
	CFLAGS += -g
	DEFINES	= -D_DEBUG -DMAME_DEBUG
	DEFINES += -Dmame_debug_init=debug_init
	DEFINES += -Dmame_debug_exit=debug_exit
	DEFINES += -DMAME_Debug=debug_main
	DEFINES += -Dmame_debug=debug_flag
#	DEFINES	+= -DVERBOSE_DEBUG
	OBJECTS	= $(WORK)/i86dasm.o $(WORK)/debug.o
else
	LFLAGS += -Xlinker -s -Xlinker -x
	TARGET	= dx
	WORK	= _Release
endif
CC	= gcc
DEFINES	+= -D__BeOS__ -D__DX__ -DLSB_FIRST \
	-DMACHTYPE=\"$(MACHTYPE)\" -DHOSTNAME=\"$(HOSTNAME)\" \
	-DBUILD=`cat build`
CFLAGS	+= -c -O9 $(DEFINES) -I$(PWD) -I$(PWD)/i86 -I$(PWD)/mame_inc
OBJECTS	+= $(WORK)/main.o $(WORK)/i86.o $(WORK)/memory.o $(WORK)/int.o $(WORK)/int21.o $(WORK)/int21_43.o $(WORK)/int21_44.o $(WORK)/int1a.o $(WORK)/file.o $(WORK)/process.o 
INSTALL	= install
RMRF	= rm -rf

all: $(TARGET)

clean:
	$(RMRF) $(WORK)

$(TARGET): $(WORK) $(OBJECTS)
	$(CC) $(LFLAGS) -o $(TARGET) $(OBJECTS)

build: $(WORK)
	echo build number inc...
	expr `cat build` + 1 > build

$(WORK):
	$(INSTALL) -d $(WORK)


$(WORK)/i86.o: i86 mame_inc memory.h
	$(CC) $(CFLAGS) -o $@ i86/i86.c

$(WORK)/i86dasm.o: i86
	$(CC) $(CFLAGS) -o $@ i86/i86dasm.c

$(WORK)/main.o: main.c i86info.h build
	$(CC) $(CFLAGS) -o $@ main.c

$(WORK)/int.o: int.c int.h
	$(CC) $(CFLAGS) -o $@ int.c

$(WORK)/int21.o: int21.c int.h file.h
	$(CC) $(CFLAGS) -o $@ int21.c

$(WORK)/int21_43.o: int21_43.c int.h
	$(CC) $(CFLAGS) -o $@ int21_43.c

$(WORK)/int21_44.o: int21_44.c int.h
	$(CC) $(CFLAGS) -o $@ int21_44.c

$(WORK)/int1a.o: int1a.c int.h
	$(CC) $(CFLAGS) -o $@ int1a.c

$(WORK)/memory.o: memory.c
	$(CC) $(CFLAGS) -o $@ memory.c

$(WORK)/file.o: file.c file.h
	$(CC) $(CFLAGS) -o $@ file.c

$(WORK)/process.o: process.c process.h file.h int.h dos.h
	$(CC) $(CFLAGS) -o $@ process.c

$(WORK)/debug.o: debug.c debug.h
	$(CC) $(CFLAGS) -o $@ debug.c

int.h:: memory.h i86info.h
process.h:: i86info.h
file.h:: dos.h

