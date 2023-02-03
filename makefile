APP = cac
FILES = main.c combat_shape.h combat_shape.c editor_history.h editor_history.c string_buffer.h string_buffer.c

run: ${FILES}
	gcc ${FILES} -o ${APP} -Wall -Werror -std=c99 -Wno-missing-braces -lraylib -lcjson -lGL -lm -lpthread -ldl -lrt -lX11
	./${APP} Jab.png

clean:
	rm ${APP}

all: run