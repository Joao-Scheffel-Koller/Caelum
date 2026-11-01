SOURCE_FILES := $(wildcard src/*.cpp src/*/*.cpp src/*/*.c src/*/*.hpp src/render/shaders/*.glsl)

./bin/Linux/main: $(SOURCE_FILES) include/* CMakeLists.txt CMakePresets.json Makefile
	cmake --preset default-config
	cmake --build --preset default-build

.PHONY: clean run
clean:
	rm -f bin/Linux/main

run: ./bin/Linux/main
	cd bin/Linux && ./main
