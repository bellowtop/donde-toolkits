## Use protoc-all docker tools to generate proto.


```
docker pull namely/protoc-all:1.47_2
docker run -v $PWD:/defs namely/protoc-all:1.47_2 -f server.proto -l cpp

# generated code dir `gen/pb-cpp/`

```

### Version Issue


I know the version of docker image is `1.47_2`, which is different from
the version we use in `conanfile.txt`: `grpc/1.40.0`, that's because the
tool generated protobuf code is min-`3019000` max-`3019004` (check server.pb.h for details)
we have to use `protobuf/3.19.1` in `conanfile.txt`.

If we use `protobuf/3.19.1`, then `grpc` cannot be too high, or it will depends
on `googleapis/cci.20220531` (from grpc/1.44.0, it started to depends on `googleapis`, god knows why!),
the `googleapis` depends on `protobuf/3.21.1`, and it conflicts with `protoc-all` generated code version!

What a mess!!!

So inclusion, we use:

* grpc/1.40.0
* protobuf/3.19.1
* docker image namely/protoc-all:1.47_2

Stick with those versions!



## Generate grpc-gateway

```
docker pull namely/gen-grpc-gateway:1.47_2
docker run -v `pwd`:/defs namely/gen-grpc-gateway:1.47_2 -f server.proto -s FaceService

# generated code in `gen/grpc-gateway`,

```


### make a small fix, maybe namely/gen-grpc-gateway:1.47_2 has a small bug?

vi gen/grpc-gateway/Dockerfile

change =>
COPY --from=build /app/gen/github.com/sekirocc/face-recognition-service/face_service/server.swagger.json /app/

to =>
COPY --from=build /app/gen/server.swagger.json /app/

```
# then build your image
docker build -t face-service-grpc-gateway gen/grpc-gateway/
```


### start the gateway http server

```
docker run -p 8080:80 -it docker.io/library/face-service-grpc-gateway --backend 192.168.31.60:9595
```

telnet localhost:8080
