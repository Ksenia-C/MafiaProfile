# sudo docker build . --tag mafia/client_chat:1.0 -f Dockerfiles/client_chat.Dockerfile
# sudo docker run --network=mafia_default --env PORT=5003 --env NAME=ksenia --env QUEUE=__base_test_queue__  -it mafia/client_chat:1.0 bash

FROM mafia/base_chat:1.0

ENTRYPOINT ["sh", "-c", "./cmake/build/client $NAME mafia-server_chat:$PORT $QUEUE"] 