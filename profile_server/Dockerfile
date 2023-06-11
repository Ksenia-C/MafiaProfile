# sudo docker build . --tag mafia/profile_server:1.0 

FROM archlinux:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN pacman -Sy --noconfirm gcc wget git ca-certificates cmake base-devel

RUN git clone https://github.com/Corvusoft/restbed.git && \
    git clone https://github.com/corvusoft/asio-dependency restbed/dependency/asio && \
    mkdir restbed/build && \
    cd restbed/build && \
    cmake -DBUILD_SSL=NO -DBUILD_TESTS=NO .. && \
    make install && \
    cp ../distribution/library/librestbed.so* /usr/lib/


RUN git clone https://github.com/libharu/libharu.git && \
    cd libharu && mkdir build && cd build && cmake .. && \
    make clean && make && make install && cd ..

RUN git clone https://github.com/SRombauts/SQLiteCpp.git && \
    cd SQLiteCpp && mkdir build && cd build && cmake .. && \
    make && make install && cd ..

COPY server.cpp server.cpp
COPY CMakeLists.txt CMakeLists.txt
COPY helpers/ helpers/
COPY FreeSerif.ttf FreeSerif.ttf
ENV LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"


RUN mkdir -p build
RUN cd build && cmake ../ && make

CMD ["sh", "-c", "./build/server $PORT"] 