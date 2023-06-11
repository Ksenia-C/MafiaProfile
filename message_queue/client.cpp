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

#include <chrono>
#include <iostream>
#include <set>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <condition_variable>
#include <algorithm>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "chat.grpc.pb.h"
#include <stdlib.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using message_queue::CharServer;
using message_queue::ClientStream;
using message_queue::ServerStream;

std::set<std::string> players;

class RouteGuideClient
{

    enum CheckStatus
    {
        NotYet,
        Normal,
        Mafia,
    };

    auto split_to_set(std::string players_list)
    {
        std::set<std::string> result;
        size_t pos = 0;
        std::string token;
        while ((pos = players_list.find('\n')) != std::string::npos)
        {
            result.insert(players_list.substr(0, pos));
            players_list.erase(0, pos + 1);
        }
        return result;
    }

public:
    RouteGuideClient(std::shared_ptr<Channel> channel)
        : stub_(CharServer::NewStub(channel))
    {
    }

    void ChatSession(const std::string &self_name, const std::string &chat_name)
    {
        ClientContext context;

        std::shared_ptr<ClientReaderWriter<ClientStream, ServerStream>> stream(
            stub_->ProcessChatSession(&context));

        ServerStream server_note;
        message_queue::ClientStream request;
        request.set_name("__connect_to_queue__");
        request.set_message(chat_name);
        stream->Write(request);

        request.set_name("__self_name__");
        request.set_message(self_name);
        stream->Write(request);

        std::thread client_input_thread = std::thread([&]()
                                                      {
            std::string message;
            request.set_name(self_name);
            while (std::getline(std::cin, message)) {
                request.set_message(message);
                stream->Write(request);
            } 
            
            stream->WritesDone(); });

        while (stream->Read(&server_note))
        {
            if (server_note.name() == self_name)
            {
                continue;
            }
            std::cout << "\033[1;34m" << server_note.name() << "\033[0m: " << server_note.message() << std::endl;
        }
        client_input_thread.join();

        Status status = stream->Finish();
        if (!status.ok())
        {
            std::cout << "RouteChat rpc failed." << std::endl;
        }
    }

private:
    std::unique_ptr<CharServer::Stub> stub_;
};

int main(int argc, char **argv)
{
    std::string get_my_name(argv[1]);
    auto address = argc > 2 ? argv[2] : "localhost:5003";
    auto queue_message = argc > 3 ? argv[3] : "__base_test_queue__";

    RouteGuideClient guide(
        grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));

    while (true)
    {
        guide.ChatSession(get_my_name, queue_message);
    }

    return 0;
}