# Face Recognition Service

Face feature extraction and searching and clustering service, built with openvino and faiss[todo] [wip].

Currently developed with macOS, llvm-clang


## Features

- [Modern CMake practices](https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/)
- Openvino inference
- Full pipeline detect/landmarks/align/extract
- Feature search service with simple store
- Feature searching with faiss - TODO
- Feature clustering with faiss - TODO
- Master/Workers architecture, Poco messaging
- Conan to manage dependencies
- Protobuf proto & GRPC server
- GRPC gateway to support http api
- Fine tests (important!)

## Dependencies

openvino, you need to install(download from openvino website) it first. other depencencies are managed by `conan/conanfile.txt`.

### Models

| name                          | type      | desc                 | urldesc                                                                                                                  |
|-------------------------------|-----------|----------------------|--------------------------------------------------------------------------------------------------------------------------|
| face-detection-adas-0001      | detect    | detect face          | https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/intel/face-detection-adas-0001/README.md  |
| facial-landmarks-35-adas-0002 | landmarks | 70 points landmarks  | https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/intel/facial-landmarks-35-adas-0002/README.md       |
| Sphereface.xml                | feature   | extract face feature | https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/public/Sphereface/README.md                         |



## Project layout


* `include` `source` are used to build `FaceRecognition` library, see `CMakelists.txt` for more details.

* `server` is used to build the server binary, which use the `FaceRecognition` lib, see `server/CMakelists.txt` for more details.

* `test` contains unit-tests, they also use the `FaceRecognition` lib, and of-cause they test it.


## Usage


### Build and run

#### Install openvino distribution

https://www.intel.com/content/www/us/en/developer/tools/openvino-toolkit/download.html

```
# double check your openvino install location, modify CMakeLists.txt if needed

set(OpenVINO_DIR "/opt/intel/openvino_2022/runtime/cmake")
find_package(OpenVINO REQUIRED COMPONENTS Runtime)
```

activate openvino env

```
python3.8 -m venv venv3.8
source venv3.8/bin/activate
source /opt/intel/openvino_2022/setupvars.sh
```


#### Install llvm clang

The project can be built use llvm clang (version 14), not use apple clang

```
brew install llvm

/usr/local/opt/llvm/bin/clang --version
Homebrew clang version 14.0.6

# explict set CC and CXX, for sure
export CC=/usr/local/opt/llvm/bin/clang ; export CXX=/usr/local/opt/llvm/bin/clang++
```

#### Build conan dependency packages

```bash
conan install --build=missing --profile conan/conanprofile  -if build ./conan
```

#### Build server binary


```bash

cmake -S server -B build/server

cmake --build build/server


# make sure export some openvino runtime variable path before running.
# source /opt/intel/openvino_2022/setupvars.sh

# run the service
./build/server/bin/FaceRecognitionServer

# with config path
./build/server/bin/FaceRecognitionServer --config_path server/examples/server.json
```



### Use Makefile

Makefile wraps `cmake` instructions together.

```
# prepare all those build dirs
make build-pre

# build and run server
make build-server
make run-server


# build and run test
make build-test
make run-test

# build lib only
make build-lib
```




### Build proto

```
cd server/protos

# build protobuf definitions of grpc and service
./build_proto.sh

# build grpc-gateway, for rest-api [optional]
./build_gateway.sh

```


## Xcode

```
mkdir -p xcode/server
cd xcode/server
cmake -G Xcode ../../server
```

then use xcode to open `FaceRecognitionServer.xcodeproj`, try build and run.



### Known Issues
Q:
CMake generate failed with message `xcode-select: error: tool 'xcodebuild' requires Xcode, but active developer directory '/Library/Developer/CommandLineTools' is a command line tools instance`

A:
Run `sudo xcode-select -s /Applications/Xcode.app/Contents/Developer`, see also `https://github.com/nodejs/node-gyp/issues/569`.


Q:
Build failed with message `"Protobuf requires at least C++11."`

A:
Navigate to `TARGETS -> API`, find `Apple Clang - Language - C++`, change `C++ Language Dialect` to `C++17[-std=c++17]`


Q:
Run failed with message `dyld[4593]: Library not loaded: @rpath/libopenvinod.dylib`

A:

It's because we need `source /opt/intel/openvino_2022/setupvars.sh`, but xcode doesn't know that.
Edit `FaceRecognitionServer` scheme, add environment variable `DYLD_LIBRARY_PATH`, value `/opt/intel/openvino_2022/runtime/3rdparty/tbb/lib::/opt/intel/openvino_2022/runtime/lib/intel64/Release:/opt/intel/openvino_2022/runtime/lib/intel64/Debug`


A2:
```
for i in `ls /opt/intel/openvino_2022/runtime/lib/intel64/Debug/`; do sudo ln -s /opt/intel/openvino_2022/runtime/lib/intel64/Debug/$i /usr/local/lib; done
for i in `ls /opt/intel/openvino_2022/runtime/3rdparty/tbb/lib/`; do sudo ln -s /opt/intel/openvino_2022.1.0.643/runtime/3rdparty/tbb/lib/$i /usr/local/lib; done
```


## Mac M1

[README mac m1](./README_mac_m1.md)
