PROG:=index.cgi
CPP_FILES:=$(wildcard src/*.cpp)
OBJ_FILES:=$(patsubst %.cpp,%.o,$(CPP_FILES))
CPPFLAGS:=-Wall -Wextra -O3

$(PROG): $(OBJ_FILES)
	$(CXX) -o $(PROG) $(notdir $(OBJ_FILES))

%.o: %.cpp
	$(CXX) -c $< $(CPPFLAGS)

clean:
	$(RM) *.o $(PROG)