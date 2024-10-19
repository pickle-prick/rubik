.PHONY: build_debug build_full run_full clean debug run

build_debug:
	./build.sh game no_shader no_meta
build_full:
	make clean
	./build.sh game shader meta 
run_full:
	make build_full
	./build/game
clean:
	rm -rf ./build
	rm ./src/api/vulkan/*.spv || true
debug:
	make build_debug
	gdb -q --tui ./build/game
run:
	make build_debug
	./build/game
