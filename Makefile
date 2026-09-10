exec = lang.exe
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -g

$(exec): $(objects)
	gcc $(objects) $(flags) -o $(exec)

%.o: %.c include/%.h
	gcc -c $(flags) $< -o $@

install:
	$(MAKE)
	powershell -Command "Copy-Item ./lang.exe C:/Windows/lang.exe -Force"

clean:
	-rm *.exe
	-rm src/*.o