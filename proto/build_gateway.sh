#! /bin/bash

docker run -v `pwd`:/defs namely/gen-grpc-gateway:1.47_2 -f server.proto -s FaceService


# change the swagger line. maybe a gen-grpc-gateway bug?
# vi gen/grpc-gateway/Dockerfile
#
# change =>
# COPY --from=build /app/gen/github.com/sekirocc/face-recognition-service/face_service/server.swagger.json /app/
#
# to =>
# COPY --from=build /app/gen/server.swagger.json /app/

sed -i -e 's|.*server.swagger.json.*|COPY --from=build /app/gen/server.swagger.json /app/|' gen/grpc-gateway/Dockerfile

rm -rf gen/grpc-gateway/Dockerfile-e

docker build -t face-service-grpc-gateway:latest  \
       --build-arg http_proxy=http://172.16.1.135:3128 \
       --build-arg https_proxy=http://172.16.1.135:3128 \
       --build-arg HTTP_PROXY=http://172.16.1.135:3128 \
       --build-arg HTTPS_PROXY=http://172.16.1.135:3128 \
       gen/grpc-gateway/
