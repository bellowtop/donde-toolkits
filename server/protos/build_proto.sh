#! /bin/bash

protoc_path=$(find ~/.conan/data/protobuf/3.21.4/_/_/package -name "protoc" | head -n 1)
grpc_cpp_plugin_path=$(find ~/.conan/data/grpc/1.48.0/_/_/package -name "grpc_cpp_plugin"  | head -n 1)
googleapis_path=$(find ~/.conan/data/grpc/1.48.0/_/_/build -name "googleapis"  | head -n 1)

mkdir -p ./gen/pb-cpp/

${protoc_path} -I${googleapis_path} -I. \
   --cpp_out=./gen/pb-cpp \
   --grpc_out=./gen/pb-cpp \
   --plugin=protoc-gen-grpc=${grpc_cpp_plugin_path} \
   server.proto
