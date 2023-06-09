build_base_container:
	sudo docker build . --tag mafia/base:1.0 -f Dockerfiles/base.Dockerfile
build_client: build_base_container
	sudo docker build . --tag mafia/client:1.0 -f Dockerfiles/client.Dockerfile

client:
	sudo docker run --init --network=mafia_default --env PORT=$(port) --env NAME=$(name) -it mafia/client:1.0 bash
