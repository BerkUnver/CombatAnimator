APP = ../build/cac
FILES = main.c layer.c editor_history.c string_buffer.c transform_2d.c
LIBRARIES = ../lib/cJSON/libcjson.a ../lib/raylib/libraylib.a

run:
	make build
	(cd "../data" && exec ./${APP} Jab.png)

clean:
	rm ${APP}

all: run

build: ${FILES}
	mkdir -p "../build"
	gcc ${FILES} ${LIBRARIES} -o ${APP} -g -Wall -Werror -std=c99 -Wno-missing-braces -fsanitize=address -fsanitize=undefined -static-libasan -I../include
