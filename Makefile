client:
	sudo docker run --init --network=mafia_default --env PORT=$(port) --env NAME=$(name) --env CHAT_PORT=$(chat_port) -it kseniac/mafia_components:client.2 bash || true
