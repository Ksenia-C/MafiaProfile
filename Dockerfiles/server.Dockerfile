# sudo docker build . --tag mafia/server:1.0 -f Dockerfiles/server.Dockerfile

FROM mafia/base:1.0

CMD ["sh", "-c", "./cmake/build/server $PORT mafia-server_chat:$CHAT_PORT"] 