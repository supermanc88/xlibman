TARGET := libxlibman.so
SRCS := xlibman.c
OBJS := $(SRCS:.c=.o)
SOOBJS := xlibman.o
CFLAGS := -Wall -g -fPIC -shared
PLATFORM := $(shell uname -m)
LIBS := -L../../output/libs/$(PLATFORM)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(SOOBJS) $(LIBS) -o $(TARGET)
	\cp -f $(TARGET) ../../output/libs/$(PLATFORM)

$(OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)