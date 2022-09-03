## How to build openvino with apple M1

openvino arm plugin build need SCons

```
brew install scons
```

clone source and build

```
cd ~

git clone https://github.com/openvinotoolkit/openvino.git
git clone https://github.com/openvinotoolkit/openvino_contrib.git

cd openvino         && git submodule update --init --recursive && cd ..
cd openvino_contrib && git submodule update --init --recursive && cd ..

mkdir openvino/build && cd openvino/build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_PYTHON=OFF -DIE_EXTRA_MODULES=~/openvino_contrib/modules/arm_plugin ..
make -j8
sudo make install

```

check supported devices.

```
➜  build git:(master) ../bin/arm64/Release/hello_query_device
[ INFO ] OpenVINO Runtime version ......... 2022.3.0
[ INFO ] Build ........... 2022.3.0-000-d9d4b6d89ba
[ INFO ]
[ INFO ] Available devices:
[ INFO ] CPU
[ INFO ] 	SUPPORTED_PROPERTIES:
[ INFO ] 		Immutable: SUPPORTED_METRICS : SUPPORTED_METRICS SUPPORTED_CONFIG_KEYS RANGE_FOR_ASYNC_INFER_REQUESTS RANGE_FOR_STREAMS
[ INFO ] 		Immutable: SUPPORTED_CONFIG_KEYS : LP_TRANSFORMS_MODE DUMP_GRAPH PERF_COUNT CPU_THROUGHPUT_STREAMS CPU_BIND_THREAD CPU_THREADS_NUM CPU_THREADS_PER_STREAM NUM_STREAMS INFERENCE_NUM_THREADS AFFINITY
[ INFO ] 		Mutable: PERF_COUNT : YES
[ INFO ] 		Immutable: AVAILABLE_DEVICES : NEON
[ INFO ] 		Immutable: FULL_DEVICE_NAME : arm_compute::NEON
[ INFO ] 		Immutable: OPTIMIZATION_CAPABILITIES : FP16 FP32
[ INFO ] 		Immutable: RANGE_FOR_ASYNC_INFER_REQUESTS : 1 10 1
[ INFO ] 		Immutable: RANGE_FOR_STREAMS : 1 10
[ INFO ] 		Mutable: CPU_THROUGHPUT_STREAMS : 1
[ INFO ] 		Mutable: CPU_BIND_THREAD : NO
[ INFO ] 		Mutable: CPU_THREADS_NUM : 0
[ INFO ] 		Mutable: CPU_THREADS_PER_STREAM : 10
[ INFO ] 		Mutable: NUM_STREAMS : 1
[ INFO ] 		Mutable: INFERENCE_NUM_THREADS : 0
[ INFO ] 		Mutable: AFFINITY : NONE
[ INFO ]
```

It will install dynamic libraries to `/usr/local/runtime/lib/arm64/Release`

now we can add run path to the our server binary.

```
cd face-recognition-service

install_name_tool -add_rpath /usr/local/runtime/lib/arm64/Release/ ./build/server/bin/FaceRecognitionServer
./build/server/bin/FaceRecognitionServer --config_path ./contrib/server.json
```


## Conan

```
export CC=/opt/homebrew/opt/llvm/bin/clang ; export CXX=/opt/homebrew/opt/llvm/bin/clang++

conan install --build=missing --profile conan/conanprofile.m1  -if build ./conan

```
