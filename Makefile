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
	conan install --build=missing --profile conan/conanprofile.m1  -if build ./conan

build-pre: conan
	cmake -B build
	cmake -S tests -B build/tests

build: build-pre
	cmake --build build      -- -j 8

build-test:
	cmake --build build/tests -- -j 8

run-test:
	./build/tests/bin/DondeToolkitsTests

.PHONY: clean all build check image

test: check
	echo 'test'

check:
	echo 'check'

clean:
	rm -rf build
