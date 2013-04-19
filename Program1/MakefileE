myshell: myshell.o lex.yy.o
	gcc -o myshell lex.yy.o myshell.o -lfl

myshell.o: myshell.c
	gcc -c myshell.c

lex.yy.o: lex.yy.c
	gcc -c lex.yy.c

lex.yy.c: lex.c
	flex lex.c

clean:
	rm myshell *.o
