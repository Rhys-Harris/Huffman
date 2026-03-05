all: build run

build:
		gcc *.c huff8/*.c huff16/*.c huff32/*.c -o "dev.exe"

run:
		dev
