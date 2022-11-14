#! /bin/bash

set -x

protoc_path=$(find ~/.conan/data/protobuf/3.21.4/_/_/package -name "protoc" | head -n 1)
grpc_cpp_plugin_path=$(find ~/.conan/data/grpc/1.48.0/_/_/package -name "grpc_cpp_plugin"  | head -n 1)

googleapis_path=$(find ~/.conan/data/googleapis/cci.20220711/_/_/build/* -name "google" -type d -maxdepth 1)/..
# like ~/.conan/data/googleapis/cci.20220711/_/_/build/a2aa3916d965a660fbb46a2d88bc1cc51651aa30/

mkdir -p ./gen/pb-cpp/
mkdir -p ../api

${protoc_path} -I${googleapis_path} -I. \
   --cpp_out=../api \
   common.proto

${protoc_path} -I${googleapis_path} -I. \
   --cpp_out=../api \
   --grpc_out=../api \
   --plugin=protoc-gen-grpc=${grpc_cpp_plugin_path} \
   feature_extract.proto

${protoc_path} -I${googleapis_path} -I. \
   --cpp_out=../api \
   --grpc_out=../api \
   --plugin=protoc-gen-grpc=${grpc_cpp_plugin_path} \
   feature_search.proto

${protoc_path} -I${googleapis_path} -I. \
   --cpp_out=../api \
   --grpc_out=../api \
   --plugin=protoc-gen-grpc=${grpc_cpp_plugin_path} \
   feature_search_inner.proto
