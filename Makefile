CC			:= $(CROSS_COMPILE)gcc
LD			:= $(CROSS_COMPILE)gcc
STRIP		:= $(CROSS_COMPILE)strip

# -Wno-multichar is used for vban.h:25 define.
CFLAGS    	:= -DLINUX -ansi -Wpedantic -Wall -Wno-multichar
LDFLAGS		:= -lasound

# Defaultly compiles release version of the application
CONFIG		:= release

ifneq ($(filter release%,$(CONFIG)),)
  LDFLAGS   += -O3
endif

ifneq ($(filter debug%,$(CONFIG)),)
  CFLAGS	+= -g
endif

SOURCES		:= 	main.c 		\
				socket.c 	\
				audio.c		\

OBJECTS		:= $(SOURCES:.c=.o)
DEPENDENCIES:= $(SOURCES:.c=.d)
EXECUTABLE	:= vban_receptor

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	@echo "Linking $@"
	@$(LD) $(LDFLAGS) -o "$@" $^
ifneq ($(filter release%,$(CONFIG)),)
	@echo "Stripping $@"
	@$(STRIP) "$@"
endif

%.o:%.c
	@echo "Compiling $<"
	@$(CC) -c $(CFLAGS) -MMD -MF "$(patsubst %.o,%.d,$@)" -o "$@" "$<"

-include $(DEPENDENCIES)

install:
	@mkdir -p $(DESTDIR)/usr/sbin
	@cp $(EXECUTABLE) $(DESTDIR)/usr/sbin

clean:
	@echo "Cleaning artifacts and objects"
	@rm -f $(OBJECTS) $(DEPENDENCIES) $(EXECUTABLE)
	@rm -f *~*

distclean: clean

