CC_64=x86_64-w64-mingw32-gcc

all: libipc.x64.zip

bin:
	mkdir bin

libipc.x64.zip: bin
	$(CC_64) -DWIN_X64 -shared -masm=intel -Wall -Wno-pointer-arith -c src/IPC.c -o bin/IPC.x64.o
	zip -q -j libipc.x64.zip bin/*.x64.o

clean:
	rm -rf bin/*.o
	rm -f libipc.x64.zip