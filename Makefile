objects = main.o

CXXFLAGS = --std=c++17

app : $(objects) -lsfml-graphics -lsfml-window -lsfml-system
	g++ -o $@ $^ $(CXXFLAGS)

main.o :

.PHONY : clean
clean :
	-rm app $(objects)
