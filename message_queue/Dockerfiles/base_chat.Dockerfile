# sudo docker build . --tag mafia/base_chat:1.0 -f Dockerfiles/base_chat.Dockerfile

FROM archlinux:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN pacman -Sy --noconfirm gcc wget git ca-certificates cmake base-devel protobuf hiredis

RUN git clone --recurse-submodules -b v1.55.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

RUN cd grpc && \
    mkdir -p cmake/build && \
    pushd cmake/build && \
    cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../.. && \
    make -j 4 && \
    make install && \
    popd

RUN git clone https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus &&  mkdir build && cd build && \
    cmake .. && make && make install && cd ..

COPY CMakeLists.txt CMakeLists.txt
COPY client.cpp client.cpp
COPY chat.proto chat.proto
COPY server.cpp server.cpp

RUN cp redis-plus-plus/build/libredis++.so* /usr/lib

RUN mkdir -p cmake/build
RUN cd cmake/build && cmake ../../ && make -j 4
RUN export TERM=xterm-256color
