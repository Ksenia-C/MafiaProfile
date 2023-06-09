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

#include "com.grpc.pb.h"
#include <stdlib.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using mafia::MafiaSimple;
using mafia::PlayerListReply;
using mafia::PlaySessionChannel;
using mafia::RegRequest;

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
        : stub_(MafiaSimple::NewStub(channel))
    {
    }

    std::optional<std::string> ListPlayers(const std::string &this_name)
    {
        mafia::RegRequest rect;
        ClientContext context;
        players.clear();

        rect.set_name(this_name);
        PlayerListReply feature;

        std::unique_ptr<ClientReader<PlayerListReply>> reader(
            stub_->ListPlayers(&context, rect));
        while (reader->Read(&feature))
        {
            if (feature.action() == "new")
            {
                players.insert(feature.name());
            }
            else if (feature.action() == "deleted")
            {
                players.erase(feature.name());
            }
            else if (feature.action() == "start")
            {
                auto my_role = feature.name();
                return my_role;
            }
            else
            {
                continue;
            }
            system("clear");
            std::cout << "Current sessions playing: " << feature.pl_session_cnt() << "\n";
            std::cout << "Players list\n";
            for (auto &name : players)
            {
                std::cout << "Player " << name << '\n';
            }
            fflush(stdout);
        }
        Status status = reader->Finish();
        if (status.ok())
        {
            std::cout << "ListPlayers rpc succeeded." << std::endl;
        }
        else
        {
            std::cout << "ListPlayers rpc failed." << std::endl;
        }
        return std::nullopt;
    }

    void cout_possible_commands(std::set<std::string> commands, const std::optional<std::string> &night_results)
    {
        system("clear");
        std::cout << "\033[1;34m" << self_name << "\033[0m\n";
        std::cout << "Role: \033[1;34m" << my_role_ << "\033[0m \t\t ";
        if (is_day_now_)
        {
            std::cout << "\033[1;33m☀\033[0m\n";
        }
        else
        {
            std::cout << "\033[1;35m☾\033[0m\n";
        }
        std::cout << "you play with them (\033[1;34malive\033[0m, \033[1;31mdead\033[0m, \033[1;35mmafia\033[0m):\n";

        for (auto &op : other_players_)
        {
            if (mafia_players_.count(op))
            {
                std::cout << "\033[1;35m" << op << "\033[0m\n";
            }
            else
            {
                std::cout << "\033[1;34m" << op << "\033[0m\n";
            }
        }
        for (auto &op : dead_players_)
        {
            std::cout << "\033[1;31m" << op << "\033[0m\n";
        }

        if (!commands.empty())
        {
            std::cout << "you may do this commands:\n";

            for (auto &com : commands)
            {
                std::cout << "\033[1;34m" << com << "\033[0m\n";
            }
        }
        else
        {
            std::cout << "wait for other players to make actions\n";
        }
        std::cout << "------------------------------------\n";
        if (night_results.has_value())
        {
            std::cout << "this night results: " << *night_results << '\n';
        }
        fflush(stdout);
    }

    auto random_from_set(const std::set<std::string> &se)
    {
        if (se.size() == 0)
        {
            return se.begin();
        }
        auto it = se.begin();
        std::advance(it, rand() % se.size());
        return it;
    }

    void AutoPlaySession()
    {
        ClientContext context;

        std::shared_ptr<ClientReaderWriter<PlaySessionChannel, PlaySessionChannel>> stream(
            stub_->PlaySession(&context));

        std::set<std::string> possible_day_commands = {"execute"};

        std::string know_mafia_members;

        std::string last_mafia_check;
        PlaySessionChannel server_note;

        mafia::PlaySessionChannel request;
        request.set_action("start_session");
        request.set_name(self_name);
        stream->Write(request);

        std::cout << "Bot's role: " << my_role_ << std::endl;

        auto day_pass = [&]()
        {
            auto next_command = random_from_set(possible_day_commands);
            if (next_command != possible_day_commands.end())
            {
                if (*next_command == "execute")
                {
                    auto victim = random_from_set(mafia_players_.size() != 0 ? mafia_players_ : other_players_);

                    request.set_action("execute");
                    request.set_name(*victim);
                    std::cout << "Bot votes for " << *victim << " execution" << std::endl;
                }
                else if (*next_command == "show_mafia")
                {
                    request.set_action("uncover_mafia");
                    request.set_name(know_mafia_members);
                    std::cout << "Bot shows mafia members: " << know_mafia_members << std::endl;
                }
                stream->Write(request);
                request.set_action("end_day");
                request.set_name("");
                stream->Write(request);

                std::cout << "Bot falls asleep" << std::endl;
            }
        };

        while (stream->Read(&server_note))
        {
            if (server_note.action() == "other_players")
            {

                auto split_result = split_to_set(server_note.name());
                other_players_.insert(split_result.begin(), split_result.end());

                day_pass();
            }
            else if (server_note.action() == "you_died")
            {
                std::cout << "Bot died " << server_note.name() << std::endl;
                possible_day_commands.clear();
                other_players_.erase(self_name);
                dead_players_.insert(self_name);
            }
            else if (server_note.action() == "choose_victim" && my_role_ == "mafia")
            {
                std::cout << "you need to choose who will die" << std::endl;
                auto victim = random_from_set(other_players_);
                request.set_action("kill");
                request.set_name(*victim);
                stream->Write(request);
                std::cout << "Bot kills " << *victim << std::endl;
            }
            else if (server_note.action() == "choose_mafia_check" && my_role_ == "police")
            {
                std::cout << "you need to choose who need to be checked" << std::endl;
                auto victim = random_from_set(other_players_);

                last_mafia_check = *victim;
                request.set_action("check");
                request.set_name(last_mafia_check);

                stream->Write(request);
            }
            else if (server_note.action() == "check_mafia")
            {
                if (server_note.name() == "yes")
                {
                    possible_day_commands.insert("show_mafia");
                    std::cout << last_mafia_check << " is member of mafia" << std::endl;
                    know_mafia_members.append(last_mafia_check).append("\n");
                }
                else
                {
                    std::cout << last_mafia_check << " is usual residence" << std::endl;
                }
            }
            else if (server_note.action() == "start_day")
            {

                std::cout << "Day started" << std::endl;
                day_pass();
            }
            else if (server_note.action() == "end_game")
            {
                auto who_win = server_note.name();
                std::cout << "Wins " << who_win << "(press any key to continue)" << std::endl;
                break;
            }
            else if (server_note.action() == "vote")
            {
                std::cout << "Reply for your vote: " << server_note.name() << std::endl;
            }
            else if (server_note.action() == "dead_player")
            {
                const std::string &name = server_note.name();
                auto pl_iter = other_players_.find(name);
                if (pl_iter != other_players_.end())
                {
                    other_players_.erase(pl_iter);
                    dead_players_.insert(name);
                    std::cout << name << " now is dead" << std::endl;
                    pl_iter = mafia_players_.find(name);
                    if (pl_iter != mafia_players_.end())
                    {
                        mafia_players_.erase(pl_iter);
                    }
                }
            }
            else if (server_note.action() == "mafia_is")
            {
                std::cout << "Mafia was discovered, known members are: " << server_note.name() << std::endl;
                auto split_result = split_to_set(server_note.name());
                mafia_players_.insert(split_result.begin(), split_result.end());
            }
            else if (server_note.action() == "mafia_kill")
            {
                std::cout << "Answer from server for kill request: " << server_note.name() << std::endl;
            }
        }

        stream->WritesDone();
        while (stream->Read(&server_note))
        {
        }

        Status status = stream->Finish();
        if (!status.ok())
        {
            std::cout << "RouteChat rpc failed." << std::endl;
        }
    }

    void PlaySession()
    {
        ClientContext context;

        std::shared_ptr<ClientReaderWriter<PlaySessionChannel, PlaySessionChannel>> stream(
            stub_->PlaySession(&context));

        std::set<std::string> possible_day_commands = {"end_day", "execute"};
        std::set<std::string> possible_night_commands;
        std::optional<std::string> night_results = std::nullopt;

        if (my_role_ == "mafia")
        {
            possible_night_commands.insert("kill");
        }
        else if (my_role_ == "police")
        {
            possible_night_commands.insert("check");
        }

        std::string know_mafia_members;

        std::string last_mafia_check;

        // std::mutex mutex;
        // std::condition_variable wait_time_of_day;
        std::atomic<bool> is_still_play{true};

        std::thread writer([&]()
                           {
            mafia::PlaySessionChannel rect;
            rect.set_action("start_session");
            rect.set_name(self_name);
            stream->Write(rect);

            auto cin_person = [&]() -> std::optional<std::string>
            {
                std::string person;
                std::cin >> person;
                if (other_players_.count(person) == 0)
                {
                    std::cout << "no such person, try again" << std::endl;
                    return std::nullopt;
                }
                return person;
            };

            std::string command;
            while (is_still_play && std::cin >> command && is_still_play)
            {
                bool is_day_now = is_day_now_.load();
                if (is_day_now && possible_day_commands.count(command))
                {
                    if (command == "end_day")
                    {
                        rect.set_action("end_day");
                        rect.set_name("");
                        night_results = std::nullopt;
                    }
                    else if (command == "execute")
                    {
                        if (auto victim = cin_person())
                        {
                            rect.set_action("execute");
                            rect.set_name(*victim);
                        } else {
                            continue;
                        }
                    }
                    else if (command == "show_mafia")
                    {
                        rect.set_action("uncover_mafia");
                        rect.set_name(know_mafia_members);
                        know_mafia_members = "";
                    }
                }
                else if (!is_day_now && possible_night_commands.count(command))
                {
                    if (command == "kill")
                    {
                        if (auto victim = cin_person())
                        {
                            rect.set_action("kill");
                            rect.set_name(*victim);
                        } else {
                            continue;
                        }
                    }
                    else if (command == "check")
                    {
                        if (auto victim = cin_person())
                        {
                            last_mafia_check = *victim;
                            rect.set_action("check");
                            rect.set_name(last_mafia_check);
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
                else
                {
                    std::cout << "sorry, i don't get you" << std::endl;
                    continue;
                }
                stream->Write(rect);
            }

            stream->WritesDone(); });

        PlaySessionChannel server_note;

        while (stream->Read(&server_note))
        {
            if (server_note.action() == "other_players")
            {

                auto split_result = split_to_set(server_note.name());
                other_players_.insert(split_result.begin(), split_result.end());

                cout_possible_commands(possible_day_commands, night_results);
            }
            else if (server_note.action() == "you_died")
            {
                std::cout << "you died " << server_note.name() << std::endl;
                possible_day_commands.clear();
                possible_night_commands.clear();
                other_players_.erase(self_name);
                dead_players_.insert(self_name);
                cout_possible_commands(possible_day_commands, night_results);
            }
            else if (server_note.action() == "choose_victim" && my_role_ == "mafia")
            {
                std::cout << "you need to choose who will die" << std::endl;
            }
            else if (server_note.action() == "choose_mafia_check" && my_role_ == "police")
            {
                std::cout << "you need to choose who need to be checked" << std::endl;
            }
            else if (server_note.action() == "check_mafia")
            {
                if (server_note.name() == "yes")
                {
                    possible_day_commands.insert("show_mafia");
                    night_results.emplace(std::string("").append(last_mafia_check).append(" is member of mafia"));

                    know_mafia_members.append(last_mafia_check).append("\n");
                }
                else
                {
                    night_results.emplace(std::string("").append(last_mafia_check).append(" is usual residence"));
                }

                std::cout << *night_results << std::endl;
            }
            else if (server_note.action() == "start_day")
            {
                is_day_now_.store(true);

                std::cout << "day started" << std::endl;
                cout_possible_commands(possible_day_commands, night_results);
            }
            else if (server_note.action() == "end_game")
            {
                auto who_win = server_note.name();
                is_still_play.store(false);
                std::cout << "wins " << who_win << "(press letter to continue)" << std::endl;
                break;
            }
            else if (server_note.action() == "vote")
            {
                std::cout << "Reply for your vote: " << server_note.name() << std::endl;
            }
            else if (server_note.action() == "start_night")
            {

                is_day_now_.store(false);
                cout_possible_commands(possible_night_commands, night_results);
            }
            else if (server_note.action() == "dead_player")
            {
                const std::string &name = server_note.name();
                auto pl_iter = other_players_.find(name);
                if (pl_iter != other_players_.end())
                {
                    other_players_.erase(pl_iter);
                    dead_players_.insert(name);
                    cout_possible_commands(possible_day_commands, night_results);
                }
            }
            else if (server_note.action() == "mafia_is")
            {
                auto split_result = split_to_set(server_note.name());
                mafia_players_.insert(split_result.begin(), split_result.end());
                cout_possible_commands(possible_day_commands, night_results);
                std::cout << "Mafia was discovered, known members are: " << server_note.name() << std::endl;
            }
            else if (server_note.action() == "mafia_kill")
            {
                night_results.emplace(std::string("").append("Answer from server for kill request: ").append(server_note.name()));
                std::cout << *night_results << std::endl;
            }
        }
        writer.join();
        Status status = stream->Finish();
        if (!status.ok())
        {
            std::cout << "RouteChat rpc failed." << std::endl;
        }
    }

    void InitSession(std::string my_name, std::string my_role)
    {
        self_name = std::move(my_name);
        my_role_ = std::move(my_role);
        other_players_.clear();
        dead_players_.clear();
        is_day_now_ = true;
    }

private:
    std::unique_ptr<MafiaSimple::Stub> stub_;

    // for session (which is one at any time for class instance)
    std::string my_role_;
    std::string self_name;
    std::atomic<bool> is_day_now_;
    std::set<std::string> other_players_;
    std::set<std::string> dead_players_;
    std::set<std::string> mafia_players_;
};

int main(int argc, char **argv)
{
    srand(time(NULL));

    std::string get_my_name(argv[1]);
    auto address = argc > 2 ? argv[2] : "localhost:5002";
    bool is_bot_play = argc > 3 && argv[3][0] == 'b';

    RouteGuideClient guide(
        grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));

    while (true)
    {
        system("clear");
        std::cout << "Players list" << std::endl;
        std::optional<std::string> my_role;
        if (my_role = guide.ListPlayers(get_my_name))
        {
            guide.InitSession(get_my_name, std::move(*my_role));
            if (is_bot_play)
            {
                guide.AutoPlaySession();
            }
            else
            {
                guide.PlaySession();
            }
        }
        else
        {
            break;
        }
    }

    return 0;
}