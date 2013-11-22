# $@ - Name of the target
# $? - List of dependencies more recent than target
# $^ - All dependencies without duplicates
# $+ - All dependencies with duplicates
# $< - First dependency
# $(@D) - The directory part of the file name of the target
# %  - Wildcard, matches text stored in $*
# OBJ = $(SRC:.c=.o) - replace .c extension with .o
# OBJS:= $(addprefix $(BUILD_DIR)/, $(OBJS)) - add prefix to list

OBJS:= main.o stat_gen.o cloud_conn.o http_conn.o s3_conn.o cf_conn.o 	\
       http_req.o logging.o
TARGET:= cloud-ping

AR:=ar

AS:=as
ASFLAGS:= -gstabs

LD:=g++
LDFALGS:=

#CC:=gcc
CC:=g++

GCCVERSION:=$(shell gcc -dumpversion |cut -f1,2 -d. --output-delimiter='0')
CFLAGS:= -g -Wall -fPIC
ifeq ($(shell [ $(GCCVERSION) -lt 407 ] && echo 1), 1)
	CFLAGS += -std=c++0x
else
	CFLAGS += -std=c++11
endif

LIBS:= -lboost_program_options -lcurl -lcrypto -lgcrypt
INCLUDES:=

BUILD_DIR:= build

OBJS:= $(addprefix $(BUILD_DIR)/, $(OBJS))
TARGET:= $(addprefix $(BUILD_DIR)/, $(TARGET))

$(TARGET): $(OBJS)
	$(LD) $(LDFALGS) -o $@ $^ $(LIBS)

$(TARGET).a: $(OBJS)
	mkdir -p $(@D)
	$(AR) rcs $@ $^

$(TARGET).so: $(OBJS)
	mkdir -p $(@D)
	$(LD) -shared -soname $@.1 -o $@.1.0 $^

-include $(OBJS:.o=.d)

$(BUILD_DIR)/%.o: %.S
	mkdir -p $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c -MD $(CFLAGS) $(INCLUDES) -o $@ $<

$(BUILD_DIR)/%.o: %.cc
	mkdir -p $(@D)
	$(CC) -c -MD $(CFLAGS) $(INCLUDES) -o $@ $<

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CC) -c -MD $(CFLAGS) $(INCLUDES) -o $@ $<

clean:
	-rm -rf $(BUILD_DIR)
