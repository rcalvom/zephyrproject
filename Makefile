
all:
	west build -b native_posix_64 test_server/


clean:
	rm -rf build


run:
	sudo PORTABILITY_LAYER_PATH=/home/rcalvome/Documents/app/rtos-portability-layer/portability_layer.so TAP_INTERFACE_NAME=tap0 ./build/zephyr/zephyr.elf