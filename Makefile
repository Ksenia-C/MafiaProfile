# client:
# 	sudo docker run --init --network=mafia_default --env PORT=$(port) --env NAME=$(name) -it kseniac/mafia_components:client bash

build_base_container:
	sudo docker build . --tag mafia/base:1.0 -f Dockerfiles/base.Dockerfile
build_client: build_base_container
	sudo docker build . --tag mafia/client:1.0 -f Dockerfiles/client.Dockerfile

client:
	sudo docker run --init --network=mafia_default --env PORT=$(port) --env NAME=$(name) --env CHAT_PORT=$(chat_port) -it mafia/client:1.0 bash || true
