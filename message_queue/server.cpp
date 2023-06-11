/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <sw/redis++/redis++.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <time.h>
#include <thread>
#include <random>

#include "chat.pb.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "chat.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using message_queue::CharServer;
using message_queue::ClientStream;
using message_queue::ServerStream;

#include <chrono>

using std::chrono::system_clock;
using namespace message_queue;
using namespace std::chrono_literals;
using namespace sw::redis;

class CharServerImpl final : public CharServer::Service
{
    using streamT = ::grpc::ServerWriter<::message_queue::ServerStream> *;
    enum AllowAction
    {
        Nothing,
        ReadOnly,
        ReadWrite
    };

public:
    CharServerImpl()
    {
        ConnectionOptions connection_options;
        connection_options.host = "redis-server";
        connection_options.port = 6379;

        connection_options.socket_timeout = std::chrono::seconds(1);

        redis_.emplace(connection_options);
    }
    ::grpc::Status SetQueueForClient(::grpc::ServerContext *context, const ::message_queue::SetQueueReq *request, ::message_queue::SimpleResponse *response)
    {
        auto man_name = request->client_name();

        if (request->subscribe_status() == "delete")
        {
            if (client_queues_.find(man_name) == client_queues_.end())
            {
                return Status::OK;
            }
            client_queues_.erase(man_name);
            client_allowd_actions_.erase(man_name);

            response->set_message("OK");
            return Status::OK;
        }
        client_queues_[man_name] = request->queue_name();
        AllowAction wh;
        if (request->subscribe_status() == "read_write")
        {
            wh = ReadWrite;
        }
        else if (request->subscribe_status() == "read_only")
        {
            wh = ReadOnly;
        }
        else
        {
            wh = Nothing;
        }
        client_allowd_actions_[request->client_name()].store(wh);

        response->set_message("OK");
        return Status::OK;
    }

    ::grpc::Status ProcessChatSession(::grpc::ServerContext *context, ::grpc::ServerReaderWriter<::message_queue::ServerStream, ::message_queue::ClientStream> *stream)
    {

        ClientStream note;
        stream->Read(&note);

        // to be honest, they may be some checker that all information is trusted
        if (note.name() != "__connect_to_queue__")
        {
            return Status::OK;
        }
        std::string chat_queue = note.message();

        stream->Read(&note);

        // to be honest, they may be some checker that all information is trusted
        if (note.name() != "__self_name__")
        {
            return Status::OK;
        }
        std::string client_name = note.message();
        if (chat_queue.substr(0, 19) == std::string("__base_test_queue__"))
        {
            client_allowd_actions_[client_name].store(AllowAction::ReadWrite);
        }
        else if (client_queues_[client_name] != chat_queue)
        {
            return Status::OK;
        }
        std::cerr << "new client" << std::endl;

        std::atomic_bool is_work{true};

        std::unique_lock lock(mutex_);
        auto sub = redis_->subscriber();
        lock.unlock();

        sub.on_message([&](std::string channel, std::string msg)
                       {
            if (client_allowd_actions_[client_name].load() == AllowAction::Nothing) {
                return;
            }
            static ServerStream response;
            size_t pos = 0;
            std::string token;
            pos = msg.find('#');
            response.set_name( msg.substr(0, pos));
            msg.erase(0, pos + 1);     
            std::cerr << " got from queue "<< msg << std::endl;   
            response.set_message(std::move(msg));

            stream->Write(response); });

        sub.subscribe(chat_queue);
        sub.consume();

        ServerStream answer;

        std::thread read_client_thr = std::thread([&]()
                                                  {
        while (stream->Read(&note))
        {
            if (client_allowd_actions_[client_name].load() != AllowAction::ReadWrite) {
                continue;
            }
            std::string cl_name = note.name();
            cl_name = cl_name.append("#").append(note.message());

            {
              lock.lock();
              redis_->publish(chat_queue, cl_name); 
              lock.unlock();
            }
        } 
        is_work.store(false); });
        while (is_work.load())
        {
            try
            {
                sub.consume();
            }
            catch (const TimeoutError &e)
            {
                continue;
            }
            catch (const Error &err)
            {
                std::cerr << "some extention " << err.what() << std::endl;
                break;
            }
        }

        sub.unsubscribe(chat_queue);
        sub.consume();

        read_client_thr.join();

        return Status::OK;
    }

private:
    std::optional<Redis> redis_;
    std::mutex mutex_;

    std::map<std::string, std::atomic<AllowAction>> client_allowd_actions_;
    std::map<std::string, std::string> client_queues_; // asume only one game for player
};

void RunServer(const std::string &port)
{
    std::string server_address(std::string("0.0.0.0:").append(port));
    CharServerImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cerr << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    auto port = argc > 1 ? argv[1] : "5003";

    RunServer(port);

    return 0;
}