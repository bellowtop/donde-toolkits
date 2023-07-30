# Donde toolkits

It's a group of AI/CV related toolkits, including face feature extraction, feature searching and clustering service. Built with openvino and faiss[todo] [wip].

Currently developed with macOS, llvm-clang


## Features

- Openvino inference
- Full pipeline detect/landmarks/align/extract
- Feature search service with simple store
- Feature searching with faiss
- Feature clustering with faiss
- Master/Workers architecture, Poco messaging
- Conan to manage dependencies
- Tests coverage

## Dependencies

openvino, you need to install(download from openvino website) it first. other depencencies are managed by `conan/conanfile.txt`.

### Models

| name                          | type      | desc                 | urldesc                                                                                                                  |
|-------------------------------|-----------|----------------------|--------------------------------------------------------------------------------------------------------------------------|
| face-detection-adas-0001      | detect    | detect face          | https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/intel/face-detection-adas-0001/README.md  |
| facial-landmarks-35-adas-0002 | landmarks | 70 points landmarks  | https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/intel/facial-landmarks-35-adas-0002/README.md       |
| Sphereface.xml                | feature   | extract face feature | https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/public/Sphereface/README.md                         |



## Project layout


* `include` contains main api headers, define what `DondeToolkits` is. include common abstract definitions for feature extract, search, video process etc.

* `src` contains implementations for api headers, and contains related header definition for those implementations.

* `test` contains tests, they test the `DondeToolkits` library. tests depend to library.


## Usage


### Build and run

#### Install openvino distribution


#### Install dependencies

```
brew install openvino
brew install opencv
```

#### For linux

please refer to `https://www.intel.com/content/www/us/en/developer/tools/openvino-toolkit/download.html`

```
#double check your openvino install location, modify CMakeLists.txt if needed

set(OpenVINO_DIR "/opt/intel/openvino_2022/runtime/cmake")
find_package(OpenVINO REQUIRED COMPONENTS Runtime)
```

activate openvino env

```
python3.8 -m venv venv3.8
source venv3.8/bin/activate
source /opt/intel/openvino_2022/setupvars.sh
```


#### Build conan dependency packages

```bash
mkdir -p build
conan install --build=missing --profile conan/conanprofile.m1  -if build ./conan
```

#### Build toolkits with testings

```bash

cmake -B build -DDondeToolkits_ENABLE_UNIT_TESTING=true -DDondeToolkits_ENABLE_EXAMPLES=true
cmake --build build
```

```
# run tests
./build/bin/DondeToolkitsTests
```

```
# run examples
./build/bin/video_decode <mp4-file>

such as:
./build/bin/video_decode /tmp/Iron_Man-Trailer_HD.mp4
```

## Mac M1

[README mac m1](./README_mac_m1.md)
