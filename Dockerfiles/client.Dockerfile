# sudo docker build . --tag mafia/client:1.0 -f Dockerfiles/client.Dockerfile
# sudo docker run --network=mafia_default --env PORT=5002 --env NAME=ksenia  -it mafia/client:1.0 bash

FROM mafia/base:1.0

ENTRYPOINT ["sh", "-c", "./cmake/build/client $NAME mafia-server:$PORT n mafia-server_chat:$CHAT_PORT"] 