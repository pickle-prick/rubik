.PHONY: build_debug build_full run_full clean debug run

build_debug:
	./build.sh rubik no_shader no_meta profile
build_full:
	make clean
	./build.sh rubik shader meta profile
run_full:
	make build_full
	./build/rubik
clean:
	rm -rf ./build
	rm ./src/render/vulkan/shader/*.spv || true
debug:
	make build_debug
	gdb -q --tui ./build/rubik
run:
	make build_debug
	./build/rubik
