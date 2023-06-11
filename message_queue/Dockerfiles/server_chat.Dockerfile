# sudo docker build . --tag mafia/server_chat:1.0 -f Dockerfiles/server_chat.Dockerfile

FROM mafia/base_chat:1.0

CMD ["sh", "-c", "./cmake/build/server $PORT"] 