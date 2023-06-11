client:
	sudo docker run --init --network=mafiachat_default --env PORT=$(port) --env NAME=$(name) --env CHAT_PORT=$(chat_port) -it kseniac/mafia_components:client.1 bash || true
