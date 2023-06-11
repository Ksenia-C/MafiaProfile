Игра в мафию на основе grpc. Есть возможность запускать клиентов и ботов, играющих на рандоме.
Однако если в игре всех человеческих участников отстранили, то боты могут доиграть достаточно быстро, что в итоге вы увидите только результат партии, возможно не с совсем корректным осотбражением состояния.


# Третья часть (rest service)

Не удалось отладить получение фото, без этой части.

В примерах сервер запускается на 5004 порту.

Примерный URL для запросов (некоторые символы заменяются на последовательность других, поэтому лучше вбивать через редакторы, например insomnia или postman):

```http://localhost:5004/player?name=ksenia&gender=female&email=kseniacom```

В виде curl (здесь по идее уже рабочий вариант):
```bash
curl --request POST --url 'http://localhost:5004/player?name=ksenia&gender=female&email=ksenia%40gmail.com'
```

```bash
curl --request POST --url 'http://localhost:5004/player?name=ksenia&gender=female&email=ksenia%40gmail.com'
```

__POST__ - добавление, __GET__ - получение, __UPDATE__ - обновление, __DELETE__ - удаление

Чтобы получить несколько людей сразу:

```bash
curl --request GET --url 'http://localhost:5004/player_list?names=vieran%2Cksenia'
  ```
где ```%2C``` - запятая.

  Чтобы сгенерировать pdf отчет, нужно перейти по ```http://localhost:5004/get_info?name=ksenia```, и после по полученной в ответе ссылке. Она будет одной и той же для одного человека, но чтобы сгенерировать новый документ нужно сначала обновить первую __get_info__ страницу.


# Прошлые readme

# некоторая подготовка

Нужно освободить 6379 порт для сервиса redis. Это дефолтный порт для него, и если вдруг он уже занят, скорее всего под этот сервис.

# Вторая часть (чат)


## Простой сервис
Перейти в папку __message_queue__ и поднять docker-compose с серверами

```
cd message_queue
sudo docker-compose up
```

Для клиентов, которые нужно запускать вручную (чтобы консоль работала), запустить из той же папки __message_queue__:

```make client name=ksenia port=5003 queue=__base_test_queue__```

```make client name=ksenia port=5003 queue=__base_test_queue__```

После можно что-нибудь написать в терминал чтобы проверить доставку сообщений. Чтобы выйти - выполнить ctrl+C

```__base_test_queue__``` - очереди в которую можно писать и читать всем, без дополнительной проверки. Чтобы создать еще одну, аналогичную, можно назвать ее ```__base_test_queue__1```.

## Для мафии
Если до этого вы проверяли просто сервис обмена сообщения или другую версию мафии, то перед дальшейними действиями нужно выполнить
```sudo docker rm redis-server mafia-server_chat mafia-server mafia-bot-1 mafia-bot-2 mafia-bot-3```. Иначе будут конфликтовать имена контейнеров. Если будут похожие конфликты, также можно добавить его в команду.



Количество игроков пришлось увеличить до 6, теперь есть 2 мафии (с пятью они выигрывают после первой ночи).
С этим в docker-compose запускается 3 бота, а еще 3 клиента нужно будет запускать вручную через

```make client name=mafia1 port=5002 chat_port=5003```

```make client name=mafia2 port=5002 chat_port=5003```

```make client name=police port=5002 chat_port=5003```

Вместо ```name=mafia1``` можно написать другое имя, но если запустить клиенты с такими именами, mafia1 и mafia2 получат роли мафии, а police - комиссара. Это почти минимальный набор чтобы проверить работоспособность ночных чатов.

Также признаки (убитые участники) не могут писать в чат, но все же видят сообщения из него - условно чтобы не было скучно.

Команды для окончания дня и других действий остаются те же (можно посмотреть ниже). Все остальные сообщения будут восприниматься как сообщения в чат. Если человек не может писать в чат, его сообщения остаются в его консоли, но другим не доставляются. К примеру возможный ввод в консоль:

```bash
execute saura # голосование за игрока с именем saura днем для исключения
What do you think about mafias choose this night?  # Весь текст на этой строчке доставится другим участниками
end_day # завершение дня
```

# Первая часть (версия из первой дз)

Мафия простая - 4 игрока, одна мафия (__mafia__), один комисар (__police__).

1. В конце каждого дня нужно выполнять __end_day__.

2. Для голосования выполните __execute__ и имя участника, которые хотите исключить днем

2. __kill__ и __check__ также требуют имя участника после них, которого хотите убить или проверить на принадлежность к мафии

# Запуск
В docker-compose.yaml описаны сервер и боты. Ботов можно добавлять в аналогичном формате уже написанных, они будут постоянно подключаться к серверу, сразу после завершения сессии игры.

```yaml
mafia-bot-k:

    build:
      context: .
      dockerfile: Dockerfiles/client_bot.Dockerfile
    depends_on:
      - mafia-server
    image: mafia/client_bot:1.0
    container_name: mafia-bot-k
    environment:
      - NAME=saura
      - PORT=5002
```

Где NAME - имя с которым бот будет подключатся к серверу, PORT - порт, который слушает сервер


Сначала выполните ```sudo docker-compose up```, так как он содержит контейнер с сервером.

Клиент разворачивается отдельным контейнером, который можно запустить командой ```make client name=ksenia port=5002 chat_port=5003```. Завершить процесс можно через сигналы (например ctrl+C, для этого сигнала make завершится с ошибкой, но это нормально).
