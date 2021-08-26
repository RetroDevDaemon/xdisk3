OBJ:=main file lz77e pch sio xcom xdisk 
CC_OPTS=-g

default: clean $(OBJ)
	g++ $(CC_OPTS) -o xdisk3 ./*.o
	rm *.o

$(OBJ): %: %.cpp 
	g++ $(CC_OPTS) -c $< -o $@.o

clean:
	rm -rf ./*.o
