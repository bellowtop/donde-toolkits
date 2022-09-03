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
	cmake -S server -B build/server
	cmake -S test   -B build/test
	cmake -S .      -B build

build-server:
	cmake --build build/server -- -j 4

build-test:
	cmake --build build/test

build-lib:
	cmake --build build

build-all: build-pre build-lib build-server build-test


run-server:
	install_name_tool -add_rpath /usr/local/runtime/lib/arm64/Release/ ./build/server/bin/FaceRecognitionServer || true
	./build/server/bin/FaceRecognitionServer --config_path ./contrib/server.json

run-test:
	./build/test/bin/FaceRecognitionTests

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
