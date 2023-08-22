BUILD_NAME = cac
BUILD_PATH := ../build/${BUILD_NAME}

FILES = main.c layer.c editor_history.c string_buffer.c transform_2d.c

OS_NAME := ${shell uname -s}

ifeq (${OS_NAME},Darwin)
    OS_LIBRARY_FOLDER := macos
    COMPILER := clang
    FRAMEWORKS := -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
else ifeq (${OS_NAME},Linux)
    OS_LIBRARY_FOLDER := linux
    COMPILER := gcc
endif

LIBRARIES := ../lib/${OS_LIBRARY_FOLDER}/cJSON/libcjson.a ../lib/${OS_LIBRARY_FOLDER}/raylib/libraylib.a

run:
	make build
	cp ${BUILD_PATH} /usr/local/bin/${BUILD_NAME}
	(cd "../data" && exec ./${BUILD_PATH} Jab.png)

clean:
	rm ${BUILD_PATH}

all: run

build: ${FILES}
	mkdir -p "../build"
	${COMPILER} ${FILES} ${LIBRARIES} ${FRAMEWORKS} -o ${BUILD_PATH} -g -Wall -Werror -std=c99 -Wno-missing-braces -fsanitize=address -fsanitize=undefined -static-libsan -I../include/raylib -I../include/cJSON

