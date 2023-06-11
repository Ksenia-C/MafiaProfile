# sudo docker build . --tag mafia/base:1.0 -f Dockerfiles/base.Dockerfile

FROM archlinux:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN pacman -Suy --noconfirm gcc wget git ca-certificates cmake base-devel protobuf

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

# RUN pacman -S --noconfirm protobuf

RUN git clone https://github.com/libcpr/cpr.git

RUN cd cpr && mkdir build && cd build && \
    cmake .. -DCPR_USE_SYSTEM_CURL=ON && \
    cmake --build . && \
    sudo cmake --install . && \
    cd ../../

COPY CMakeLists.txt CMakeLists.txt
COPY client.cpp client.cpp
COPY com.proto com.proto
COPY server.cpp server.cpp

RUN mkdir message_queue 
COPY message_queue/chat.proto message_queue/chat.proto
RUN protoc --grpc_out message_queue/ --cpp_out message_queue/ -I message_queue/ --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin message_queue/chat.proto 


RUN mkdir -p cmake/build
RUN cd cmake/build && cmake ../../ && make -j 4
RUN export TERM=xterm-256color
