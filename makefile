run: main.c combat_shape.h combat_shape.c editor_history.c editor_history.h
	gcc combat_shape.c combat_shape.h editor_history.c editor_history.h main.c -o app -Wall -Werror -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	./app

clean:
	rm app