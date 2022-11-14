IMAGE_FLAGS :=



VERSION := w1.1.0
BUILD := $(shell git rev-parse --short HEAD)

ifneq ($(host), )
BUILD := $(BUILD)-$(host)
endif

ifneq ($(device), none)
ifneq ($(device), )
BUILD := $(BUILD)-$(device)
endif
endif


all: check conan build


conan:
	conan install --build=missing --profile conan/conanprofile  -if build ./conan


build-pre: conan
	cmake -S servers -B build/servers
	cmake -S tests   -B build/tests
	cmake            -B build
	cmake -S proto   -B build/proto

build-proto:
	cd proto && ./build_proto.sh

build-server:
	cmake --build build/servers -- -j 8

build-test:
	cmake --build build/tests   -- -j 8

build-lib:
	cmake --build build         -- -j 8

build-all: build-pre build-lib build-servers build-tests


run-server:
	install_name_tool -add_rpath /usr/local/runtime/lib/arm64/Release/ ./build/servers/bin/FaceRecognitionServer || true
	./build/servers/bin/FaceRecognitionServer --config_path ./contrib/server.json

run-test:
	./build/tests/bin/FaceRecognitionTests

image:
	docker build $(IMAGE_FLAGS) -f ./Dockerfile -t face-recognition-service:$(VERSION)-$(BUILD) .

push: image
	docker push face-recognition-service:$(VERSION)-$(BUILD)

.PHONY: clean all build check image

test: check
	echo 'test'

check:
	echo 'check'

clean:
	rm -rf build
