major2: major2.o func.o
	gcc -o major2 major2.o func.o

major2.o: major2.c
	gcc -c major2.c

func.o: func.c
	gcc -c func.c

clean:
	rm *.o major2
