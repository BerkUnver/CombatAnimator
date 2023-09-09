FILES = main.c layer.c editor_history.c string_buffer.c transform_2d.c list.c

ifeq (${OS},Windows_NT)
    BUILD_NAME := cac.exe
    OS_PATH := windows
	# We install it here because it is a default part of PATH
    INSTALL_PATH := C:/Windows
    COMPILER := gcc
    LIBRARIES_EXTERNAL := -lwinmm -lgdi32 -luser32 -lshell32
else
    OS_NAME := ${shell uname -s}
    INSTALL_PATH := /usr/local/bin
    BUILD_NAME := cac
    ifeq (${OS_NAME},Linux)
        OS_PATH := linux
        COMPILER := gcc
        STATIC_LIBASAN := -static-libasan
    else ifeq (${OS_NAME},Darwin)
        OS_PATH := macos
        COMPILER := clang
        LIBRARIES_EXTERNAL := -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
        STATIC_LIBASAN := -static-libsan
    endif
    SANITIZERS := -fsanitize=address -fsanitize=undefined ${STATIC_LIBASAN}
endif
LIBRARIES := ../lib/${OS_PATH}/cJSON/libcjson.a ../lib/${OS_PATH}/raylib/libraylib.a
BUILD_PATH := ../build/${BUILD_NAME}

run:
	make build
	cp ${BUILD_PATH} ${INSTALL_PATH}/${BUILD_NAME}
	(cd "../data" && exec ./${BUILD_PATH} Jab.png)

clean:
	rm ${BUILD_PATH}

all: run

build: ${FILES}
	mkdir -p "../build"
	${COMPILER} ${FILES} ${LIBRARIES} ${LIBRARIES_EXTERNAL} -o ${BUILD_PATH} -g -Wall -Werror -std=c99 -Wno-missing-braces ${SANITIZERS} -I../include/raylib -I../include/cJSON

