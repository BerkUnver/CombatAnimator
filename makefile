run: main.c hitbox.h hitbox.c editor_history.c editor_history.h
	gcc hitbox.c hitbox.h editor_history.c editor_history.h main.c -o app -Wall -Werror -std=c99 -Wno-missing-braces -I include/ -L lib/ -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	./app

clean:
	rm app