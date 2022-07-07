CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 -std=gnu++17

OBJS =		AudioMgr.o logger.o main.o

LIBS =

TARGET =	APlay

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
