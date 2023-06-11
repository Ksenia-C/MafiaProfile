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

#include "com.pb.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "com.grpc.pb.h"
#include <grpcpp/client_context.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using mafia::MafiaSimple;
using mafia::PlaySessionChannel;
using mafia::RegRequest;
#include <chrono>

using std::chrono::system_clock;
using namespace mafia;
using namespace std::chrono_literals;

const int MIN_PLAYER_GAME = 6;

#include "message_queue/chat.grpc.pb.h"
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using message_queue::CharServer;
using message_queue::SetQueueReq;
using message_queue::SimpleResponse;

std::string g_chat_service_address;
class OneSession
{
    enum WhoWim
    {
        NobodyYet,
        Mafia,
        Civil
    };

    WhoWim check_end()
    {
        int mafia_cnt = 0;
        int civil_cnt = 0;
        for (auto &pl : players)
        {
            if (is_alive[pl])
            {
                if (roles[pl] == "mafia")
                {
                    mafia_cnt++;
                }
                else
                {
                    civil_cnt++;
                }
            }
        }
        std::cerr << "mafia_cnt " << mafia_cnt << " civil_cnt " << civil_cnt << ' ';
        if (mafia_cnt == 0)
        {
            std::cerr << "civil wins" << std::endl;
            return WhoWim::Civil;
        }
        else if (mafia_cnt == civil_cnt || civil_cnt == 0)
        {
            std::cerr << "mafia wins" << std::endl;
            return WhoWim::Mafia;
        }
        else
        {
            std::cerr << "no yet" << std::endl;
            return WhoWim::NobodyYet;
        }
    }

    std::unique_ptr<message_queue::CharServer::Stub> chat_stub_;

    void
    setChannels(const std::string &name, const std::string &what_to_do)
    {
        grpc::ClientContext chat_context_;

        SetQueueReq server_requeset;
        server_requeset.set_client_name(name);
        server_requeset.set_subscribe_status(what_to_do);
        server_requeset.set_queue_name(session_id_);

        SimpleResponse ignore;
        Status status = chat_stub_->SetQueueForClient(&chat_context_, server_requeset, &ignore);
        if (!status.ok())
        {
            std::cout << "Some problems with message queue server." << std::endl;
        }
    }

public:
    OneSession(const std::string &session_id, std::vector<std::string> players_)
    {
        session_id_ = session_id;
        players = std::move(players_);

        int mafia_cnt = 0;
        int police_cnt = 0;

        for (auto name : players)
        {
            if (name.substr(0, 5) == "mafia")
            {
                roles[name] = "mafia";
                mafia_cnt++;
            }
            else if (name == "police")
            {
                police_cnt++;
                roles[name] = "police";
            }
            else
            {
                roles[name] = "civil";
            }
            is_alive[name] = true;
        }
        int mafia1, mafia2;
        for (int i = 0; i < players.size(); ++i)
        {
            if (roles[players[i]] == "mafia")
            {
                mafia1 = i;
                for (int j = i; j < players.size(); ++j)
                {
                    if (roles[players[j]] == "mafia")
                    {
                        mafia2 = j;
                        break;
                    }
                }
                break;
            }
        }

        if (mafia_cnt == 0)
        {
            mafia1 = rand() % players.size();
            roles[players[mafia1]] = "mafia";

            if (mafia_cnt != 2)
            {
                mafia2 = rand() % players.size();
                if (mafia2 == mafia1)
                {
                    mafia2 = (mafia2 + 1) % players.size();
                }
                roles[players[mafia2]] = "mafia";
            }
        }

        if (police_cnt == 0)
        {
            int police = rand() % players.size();
            while (police == mafia1 || police == mafia2)
            {
                police = (police + 1) % players.size();
            }
            roles[players[police]] = "police";
        }

        grpc::ClientContext context;

        auto channel = grpc::CreateChannel(g_chat_service_address, grpc::InsecureChannelCredentials());
        chat_stub_ = CharServer::NewStub(channel);

        // create chats
        for (auto &player : players)
        {
            setChannels(player, "read_write");
        }
    }

    std::string &get_role_of(const std::string &player)
    {
        return roles[player];
    }

    void WriteToPlayer(std::string player, PlaySessionChannel answer)
    {
        std::cerr << "send: " << answer.action() << ' ' << answer.name() << std::endl;

        while (!all_streams_spin_lock[player].exchange(false))
        {
        }
        all_streams[player]->Write(answer);
        all_streams_spin_lock[player].store(true);
    }

    void PlaySession(::grpc::ServerContext *context, ::grpc::ServerReaderWriter<::mafia::PlaySessionChannel, ::mafia::PlaySessionChannel> *stream,
                     const std::string &player)
    {
        PlaySessionChannel note;

        std::set<std::string> possible_day_requests = {"start_session", "end_day", "execute"};
        std::set<std::string> possible_night_requests = {"kill", "check"};

        std::map<std::string, std::function<void(const PlaySessionChannel &)>> command_act;
        PlaySessionChannel answer;

        if (roles[player] == "mafia")
        {
            mafia_pls.push_back(player);
        }
        else if (roles[player] == "police")
        {
            police_pl = player;
        }
        all_streams[player] = stream;
        all_streams_spin_lock[player].store(true);

        answer.set_action("other_players");
        std::string pall_players;
        for (auto &pl : players)
        {
            if (pl != player)
            {
                pall_players.append(pl).append("\n");
            }
        }
        answer.set_name(pall_players);

        // stream->Write(answer);
        WriteToPlayer(player, answer);

        auto send_member_is_dead = [&](const std::string &dead_person, const std::string &how)
        {
            setChannels(dead_person, "read_only");

            answer.set_action("you_died");
            answer.set_name(how);
            alive--;
            is_alive[dead_person] = false;
            exec_list.clear();

            // all_streams[dead_person]->Write(answer);
            WriteToPlayer(dead_person, answer);

            answer.set_action("dead_player");
            answer.set_name(dead_person);

            for (auto [n, str] : all_streams)
            {
                if (!is_alive[n])
                {
                    continue;
                }
                // str->Write(answer);
                WriteToPlayer(n, answer);
            }
        };

        command_act["end_day"] = [this, &player, stream, &send_member_is_dead](const PlaySessionChannel &note)
        {
            PlaySessionChannel answer;
            auto day_count_ = day_end_count.fetch_add(1) + 1;
            if (day_count_ == alive)
            {
                is_day = false;
                day_index += 1;
                day_end_count.store(0);
                waiting_cnt.store(0);
                was_voted.clear();

                for (auto &pl : players)
                {
                    if (is_alive[pl] == true)
                    {
                        if (roles[pl] == "mafia" || roles[pl] == "police")
                        {
                            waiting_cnt.fetch_add(1);
                        }
                    }
                }

                std::optional<std::string> executing;
                for (auto &pl : players)
                {
                    if (exec_list[pl] * 2 > alive)
                    {
                        executing = pl;
                        break;
                    }
                }

                if (executing.has_value())
                {
                    send_member_is_dead(*executing, "by vote");
                }
                WhoWim end_of_game = check_end();
                if (end_of_game != WhoWim::NobodyYet)
                {
                    answer.set_action("end_game");
                    answer.set_name(end_of_game == WhoWim::Mafia ? "mafia" : "civil");

                    for (auto [name, str] : all_streams)
                    {
                        // str->Write(answer);
                        WriteToPlayer(name, answer);
                    }

                    return;
                }

                for (auto &player : players)
                {
                    if (is_alive[player] && roles[player] != "mafia")
                    {
                        setChannels(player, "nothing");
                    }
                }

                answer.set_action("start_night");
                answer.set_name("");
                for (auto [n, str] : all_streams)
                {
                    // str->Write(answer);
                    WriteToPlayer(n, answer);
                }

                for (auto &mafia_pl : mafia_pls)
                {
                    if (is_alive[mafia_pl])
                    {
                        answer.set_action("choose_victim");
                        answer.set_name("");

                        // mafia_stream->Write(answer);
                        WriteToPlayer(mafia_pl, answer);
                    }
                }

                if (is_alive[police_pl])
                {
                    answer.set_action("choose_mafia_check");
                    answer.set_name("");

                    // police_stream->Write(answer);
                    WriteToPlayer(police_pl, answer);
                }
            }
        };

        command_act["execute"] = [this, &player, stream](const PlaySessionChannel &note)
        {
            PlaySessionChannel answer;
            if (day_index == 0)
            {
                answer.set_action("vote");
                answer.set_name("not possible in first day");
            }
            else if (was_voted.count(player))
            {
                answer.set_action("vote");
                answer.set_name("you've already voted");
            }
            else
            {
                exec_list[note.name()] += 1;
                answer.set_action("vote");
                answer.set_name("recorded");
                was_voted.insert(player);
            }

            // stream->Write(answer);
            WriteToPlayer(player, answer);
        };

        command_act["uncover_mafia"] = [this, &player, stream](const PlaySessionChannel &note)
        {
            PlaySessionChannel answer;
            answer.set_action("mafia_is");
            answer.set_name(note.name());
            for (auto [n, str] : all_streams)
            {
                // str->Write(answer);
                WriteToPlayer(n, answer);
            }
        };

        command_act["kill"] = [this, &player, stream](const PlaySessionChannel &note)
        {
            PlaySessionChannel answer;
            answer.set_action("mafia_kill");

            if (is_day)
            {
                answer.set_name("it's day right now, you, genius of mafia band");
            }
            else if (roles[player] != "mafia")
            {
                answer.set_name("well, you had to fight evil, but ... can I be your partner in crime?");
            }
            if (was_voted.count(player))
            {
                answer.set_name("you've already killed someone, dude");
            }
            else
            {
                victim_this_night = note.name();
                waiting_cnt.fetch_sub(1);
                answer.set_name("sure");
                was_voted.insert(player);
            }

            // stream->Write(answer);
            WriteToPlayer(player, answer);
        };

        command_act["check"] = [this, &player, stream, &possible_day_requests](const PlaySessionChannel &note)
        {
            PlaySessionChannel answer;
            answer.set_action("check_mafia");
            if (is_day)
            {
                answer.set_name("it's day right now, mafia is working as a butler so don't disturb her");
            }
            else if (was_voted.count(player))
            {
                answer.set_name("you've already checked someone");
            }
            else if (roles[player] != "police")
            {
                answer.set_name("and i said no, you know, like a liar, pseudo police •`_´•");
            }
            else if (roles[note.name()] == "mafia")
            {
                answer.set_name("yes");
                possible_day_requests.insert("uncover_mafia");
                was_voted.insert(player);
                waiting_cnt.fetch_sub(1);
            }
            else
            {
                answer.set_name("no");
                was_voted.insert(player);
                waiting_cnt.fetch_sub(1);
            }

            // stream->Write(answer);
            WriteToPlayer(player, answer);
        };

        while (stream->Read(&note))
        {
            std::cerr << "receive: " << note.action() << ' ' << note.name() << std::endl;
            answer.set_action("don't get you");
            answer.set_name("");

            if (is_day && possible_day_requests.count(note.action()))
            {
                (command_act[note.action()])(note);
            }
            else if (!is_day && possible_night_requests.count(note.action()))
            {
                (command_act[note.action()])(note);
                std::cout << waiting_cnt.load() << std::endl;
                if (waiting_cnt.load() == 0)
                {
                    send_member_is_dead(victim_this_night, "by mafia");

                    for (auto &player : players)
                    {
                        if (is_alive[player] && roles[player] != "mafia")
                        {
                            setChannels(player, "read_write");
                        }
                    }

                    answer.set_action("start_day");
                    day_end_count.store(0);
                    was_voted.clear();
                    is_day = true;
                    for (auto [name, str] : all_streams)
                    {
                        // str->Write(answer);
                        WriteToPlayer(name, answer);
                    }
                }

                WhoWim end_of_game = check_end();

                if (end_of_game != WhoWim::NobodyYet)
                {
                    setChannels(player, "delete");

                    answer.set_action("end_game");
                    answer.set_name(end_of_game == WhoWim::Mafia ? "mafia" : "civil");

                    for (auto [name, str] : all_streams)
                    {
                        // str->Write(answer);
                        WriteToPlayer(name, answer);
                    }
                    return;
                }
            }
        }
    }

private:
    std::string session_id_;
    std::vector<std::string> players;

    std::map<std::string, std::string> roles;
    std::map<std::string, int> exec_list; // how many people vote for smb
    std::map<std::string, int> is_alive;
    std::atomic<int> day_end_count{0};
    bool is_day = true;
    int day_index = 0;
    int alive = MIN_PLAYER_GAME;
    std::string victim_this_night;
    std::atomic<int> waiting_cnt{0};
    std::set<std::string> was_voted;

    std::vector<std::string> mafia_pls;
    std::string police_pl;
    std::map<std::string,
             ::grpc::ServerReaderWriter<::mafia::PlaySessionChannel, ::mafia::PlaySessionChannel> *>
        all_streams;
    std::map<std::string, std::atomic<bool>> all_streams_spin_lock;
};

class CharServerImpl final : public MafiaSimple::Service
{
    using streamT = ::grpc::ServerWriter<::mafia::PlayerListReply> *;

    bool write_to_stream(std::string player, streamT stream, PlayerListReply &reply)
    {
        while (new_pl_streams_spin_lock[player].exchange(true))
        {
        }
        auto result = stream->Write(reply);
        new_pl_streams_spin_lock[player].store(false);
        return result;
    }

public:
    grpc::Status ListPlayers(::grpc::ServerContext *context, const ::mafia::RegRequest *request, ::grpc::ServerWriter<::mafia::PlayerListReply> *writer)
    {
        std::map<std::string, streamT> mb_new_session;
        auto my_name = request->name();
        std::unique_lock lock_pl(mutex_);

        std::set<std::string> deleted_names;
        new_pl_streams_spin_lock[my_name].store(false);

        for (auto &[name, stream] : new_pl_streams)
        {
            PlayerListReply reply;
            reply.set_action("new");
            reply.set_name(my_name);
            reply.set_pl_session_cnt(sessions.size());

            if (!write_to_stream(name, stream, reply))
            {
                deleted_names.insert(name);
                new_pl_streams.erase(name);
                new_pl_streams_spin_lock.erase(name);
            }
            else
            {
                reply.set_name(name);
                write_to_stream(my_name, writer, reply);
            }
        }
        while (!deleted_names.empty())
        {
            std::set<std::string> is_case_new_deleted_names;
            for (auto &[name, stream] : new_pl_streams)
            {
                PlayerListReply reply;
                reply.set_action("deleted");
                reply.set_pl_session_cnt(sessions.size());

                for (auto &deleted_name : deleted_names)
                {
                    reply.set_name(my_name);
                    if (!write_to_stream(name, stream, reply))
                    {
                        is_case_new_deleted_names.insert(name);
                        new_pl_streams.erase(name);
                        new_pl_streams_spin_lock.erase(name);
                    }
                }
            }
            deleted_names.swap(is_case_new_deleted_names);
        }
        new_pl_streams[my_name] = writer;

        PlayerListReply reply;
        reply.set_action("new");
        reply.set_name(my_name);
        reply.set_pl_session_cnt(sessions.size());

        write_to_stream(my_name, writer, reply);

        for (auto &[name, _] : new_pl_streams)
        {
            std::cerr << my_name << ' ' << name << std::endl;
        }

        if (new_pl_streams.size() == MIN_PLAYER_GAME)
        {
            mb_new_session.swap(new_pl_streams);
            new_pl_streams.clear();
        }
        lock_pl.unlock();

        if (mb_new_session.size() == MIN_PLAYER_GAME)
        {
            std::cerr << "start new session" << std::endl;
            auto session_id = std::to_string(rand());

            std::vector<std::string> players;
            for (auto &[name, _] : mb_new_session)
            {
                players.push_back(name);
            }
            // auto session_iter = sessions.emplace(session_id, {session_id, std::move(players)}).first;

            auto session_iter = sessions.emplace(std::piecewise_construct,
                                                 std::forward_as_tuple(session_id),
                                                 std::forward_as_tuple(session_id, std::move(players)))
                                    .first;

            for (auto &[name, stream] : mb_new_session)
            {
                PlayerListReply reply;
                reply.set_action(std::string("start").append(session_id));
                reply.set_name(session_iter->second.get_role_of(name));
                write_to_stream(name, stream, reply);
                session_map[name] = session_id;
            }
            return ::grpc::Status(::grpc::StatusCode::OK, "");
        }

        while (true)
        {
            std::this_thread::sleep_for(5s);
            if (session_map.count(my_name))
            {
                return ::grpc::Status(::grpc::StatusCode::OK, "");
            }
            PlayerListReply reply;
            reply.set_action("empty");

            if (!write_to_stream(my_name, writer, reply))
            {
                reply.set_action("deleted");
                reply.set_name(my_name);
                reply.set_pl_session_cnt(sessions.size());

                std::lock_guard lock(mutex_);
                auto my_name_iter = new_pl_streams.find(my_name);
                if (my_name_iter == new_pl_streams.end() || my_name_iter->second != writer)
                {
                    return ::grpc::Status(::grpc::StatusCode::OK, "");
                }
                new_pl_streams.erase(my_name_iter);
                new_pl_streams_spin_lock.erase(my_name);

                for (auto &[name, stream] : new_pl_streams)
                {
                    write_to_stream(name, stream, reply);
                }
                return ::grpc::Status(::grpc::StatusCode::OK, "");
            }
        }
    }

    ::grpc::Status
    PlaySession(::grpc::ServerContext *context, ::grpc::ServerReaderWriter<::mafia::PlaySessionChannel, ::mafia::PlaySessionChannel> *stream)
    {

        PlaySessionChannel note;
        stream->Read(&note);
        std::string player = note.name(); // to be honest, they may be some checker that all information is trusted

        auto find_session_iter = session_map.find(player);
        if (find_session_iter != session_map.end())
        {
            auto session_class_iter = sessions.find(find_session_iter->second);
            if (session_class_iter != sessions.end())
            {
                people_in_sessions[find_session_iter->second].fetch_add(1);
                session_class_iter->second.PlaySession(context, stream, player);
                session_map.erase(find_session_iter);
                while (stream->Read(&note))
                {
                }
                people_in_sessions[find_session_iter->second].fetch_sub(1);
                if (people_in_sessions[find_session_iter->second].load() == 0)
                {
                    people_in_sessions.erase(find_session_iter->second);
                    sessions.erase(session_class_iter);
                }
                return Status::OK;
            }
            session_map.erase(find_session_iter);
        }

        PlaySessionChannel answer;
        answer.set_action("end_game");
        answer.set_name("sorry - you session wasn't found");
        stream->Write(answer);

        return Status::OK;
    }

private:
    std::mutex mutex_;
    std::map<std::string, streamT> new_pl_streams;
    std::map<std::string, std::atomic_bool> new_pl_streams_spin_lock;

    std::map<std::string, std::string> session_map;
    std::map<std::string, OneSession> sessions;
    std::map<std::string, std::atomic_int> people_in_sessions;
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

    auto port = argc > 1 ? argv[1] : "5002";
    g_chat_service_address = argc > 2 ? argv[2] : "localhost:5003";

    RunServer(port);

    return 0;
}