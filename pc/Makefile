OBJ:=main file lz77e pch sio xcom xdisk 

default: clean $(OBJ)
	g++ -g -o xdisk3 ./*.o

$(OBJ): %: %.cpp 
	g++ -g -c $< -o $@.o

clean:
	rm -rf ./*.o
