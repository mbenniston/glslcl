
STANDARD=-std=c11
CFLAGS=-Wall ${STANDARD}
CLINKS=-lglfw3 -lX11 -lXxf86vm -lXrandr -pthread -lXi -ldl -lm 
CC=cc

./bin/glslcl : ./src/main.c ./obj/glad.o
	${CC} ./src/main.c ./obj/glad.o -o ./bin/glslcl ${CFLAGS} ${CLINKS}

./obj/glad.o : ./src/glad.c
	${CC} -c ./src/glad.c -o ./obj/glad.o ${CFLAGS}

clean : 
	rm ./bin/glslcl
	rm ./obj/glad.o

install : ./bin/glslcl
	cp ./bin/glslcl /usr/bin/glslcl

uninstall : 
	rm /usr/bin/glslcl

test : ./bin/glslcl
	./bin/glslcl ./tests/* -v
