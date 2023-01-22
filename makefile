run: main.c
	gcc main.c -o app -Wall -Werror -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	./app