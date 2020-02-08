all:
	gcc -m32 -std=c99 -Wall src/*.c -o jvm.exe -lm

test:
	./jvm.exe examples/LongCode.class -c -b > examples/LongCode.output.txt
	./jvm.exe examples/HelloWorld.class -c -b > examples/HelloWorld.output.txt
	./jvm.exe examples/NameTest.class -c -b > examples/NameTest.output.txt
	./jvm.exe examples/HelloWorld.class -e > examples/hw.output
	./jvm.exe test-files/double_aritmetica.class -e > examples/double.output
