# sudo docker build . --tag mafia/client_bot:1.0 -f Dockerfiles/client_bot.Dockerfile

FROM mafia/base:1.0

CMD ["sh", "-c", "./cmake/build/client $NAME mafia-server:$PORT b mafia-server_chat:$CHAT_PORT"] 