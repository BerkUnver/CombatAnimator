APP = ../build/cac
FILES = main.c layer.c editor_history.c string_buffer.c transform_2d.c

run:
	make build
	(cd "../data" && exec ./${APP} Jab.png)

clean:
	rm ${APP}

all: run

build: ${FILES}
	mkdir -p "../build"
	gcc ${FILES} -o ${APP} -g -Wall -Werror -fsanitize=address -fsanitize=undefined -std=c99 -Wno-missing-braces -I../include -lraylib -lcjson -lGL -lm -lpthread -ldl -lrt -lX11
