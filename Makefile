exec = alang.exe
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -g
libs = -lm

$(exec): $(objects)
	gcc $(objects) $(flags) $(libs) -o $(exec)

%.o: %.c include/%.h
	gcc -c $(flags) $< -o $@

install:
	$(MAKE)
	powershell -Command "Copy-Item ./lang.exe C:/Windows/alang.exe -Force"

clean:
	-rm *.exe
	-rm src/*.o
