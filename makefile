APP = cac

run: main.c combat_shape.h combat_shape.c editor_history.c editor_history.h
	gcc combat_shape.c combat_shape.h editor_history.c editor_history.h main.c -o ${APP} -Wall -Werror -std=c99 -Wno-missing-braces -lraylib -lcjson -lGL -lm -lpthread -ldl -lrt -lX11
	./${APP} Jab.png

clean:
	rm ${APP}

all: run