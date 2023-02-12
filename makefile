APP = cac
FILES = main.c combat_shape.h combat_shape.c editor_history.h editor_history.c string_buffer.h string_buffer.c transform_2d.h transform_2d.c

run:
	make build
	./${APP} Jab.png

clean:
	rm ${APP}

all: run

build: ${FILES}
	gcc ${FILES} -o ${APP} -g -Wall -Werror -std=c99 -Wno-missing-braces -lraylib -lcjson -lGL -lm -lpthread -ldl -lrt -lX11
