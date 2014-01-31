release: out/release/v8-shell
debug:   out/debug/v8-shell
all:     release debug

clean:
	rm -r out

out/release/v8-shell: v8-shell.m.cpp
	mkdir -p out/release
	$(CXX) -std=c++11 \
               -I$(V8_DIR)/include \
               v8-shell.m.cpp \
               -L$(V8_DIR)/out/x64.release/obj.target/tools/gyp \
               -lv8_base.x64 -lv8_snapshot -lv8_nosnapshot.x64 \
               -L$(V8_DIR)/out/x64.release/obj.target/third_party/icu \
               -licui18n -licuuc -licudata \
               -pthread -lrt \
               -o out/release/v8-shell

out/debug/v8-shell: v8-shell.m.cpp
	mkdir -p out/debug
	$(CXX) -g -std=c++11 \
               -I$(V8_DIR)/include \
               v8-shell.m.cpp \
               -L$(V8_DIR)/out/x64.debug/obj.target/tools/gyp \
               -lv8_base.x64 -lv8_snapshot -lv8_nosnapshot.x64 \
               -L$(V8_DIR)/out/x64.debug/obj.target/third_party/icu \
               -licui18n -licuuc -licudata \
               -pthread -lrt \
               -o out/debug/v8-shell

.PHONY = release debug all clean

