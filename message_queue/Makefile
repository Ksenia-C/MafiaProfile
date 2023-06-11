run:
	g++ -std=c++17  -o main server.cpp -lredis++ -lhiredis -pthread -v


build_base_container:
	sudo docker build . --tag mafia/base_chat:1.0 -f Dockerfiles/base_chat.Dockerfile
build_client: build_base_container
	sudo docker build . --tag mafia/client_chat:1.0 -f Dockerfiles/client_chat.Dockerfile

client:
	sudo docker run --init --network=message_queue_default --env PORT=$(port) --env NAME=$(name) --env QUEUE=$(queue) -it mafia/client_chat:1.0 bash || true
