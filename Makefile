wol-remapper: wol-remapper.o
	gcc -Wall -o wol-remapper wol-remapper.o

wol-remapper.o: wol-remapper.c
	gcc -Wall -g -c wol-remapper.c

clean :
	rm *.o wol-remapper
