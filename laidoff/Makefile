all: debug release

debug:
	mkdir -p build-debug
	cd build-debug && cmake .. && make -j

release:
	mkdir -p build-release
	cd build-release && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j

clean: clean-debug clean-release

clean-debug:
	rm -rf build-debug

clean-release:
	rm -rf build-release

