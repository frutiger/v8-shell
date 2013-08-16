CXX = clang++

release: out/release/v8-shell
debug:   out/debug/v8-shell
all:     release debug

clean:
	rm -r out

out/release/v8-shell: v8-shell.m.cpp
	mkdir -p out/release
	$(CXX) -std=c++11 -stdlib=libc++ \
               -I$(V8_DIR)/include \
               -L$(V8_DIR)/out/x64.release -lv8_base.x64 -lv8_nosnapshot.x64 \
               v8-shell.m.cpp -o out/release/v8-shell

out/debug/v8-shell: v8-shell.m.cpp
	mkdir -p out/debug
	$(CXX) -g -std=c++11 -stdlib=libc++ \
               -I$(V8_DIR)/include \
               -L$(V8_DIR)/out/x64.debug -lv8_base.x64 -lv8_nosnapshot.x64 \
               v8-shell.m.cpp -o out/debug/v8-shell

.PHONY = release debug all clean

