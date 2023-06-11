#include <iostream>
#include <math.h>
#include <memory>
#include <cstdlib>
#include <restbed>
#include <cstring>
#include <fstream>

#include "helpers/db_client.hpp"
#include "helpers/pdf_creator.hpp"

using namespace std;
using namespace restbed;

std::unique_ptr<DBClient> g_db;
std::unique_ptr<PDFCreator> g_pdf_creator;
int g_port;

void post_pl_info_method_handler(const shared_ptr<Session> session)
{
    const auto request = session->get_request();

    try
    {
        int content_length = request->get_header("Content-Length", 0);
        std::string name = request->get_query_parameter("name");
        if (name.empty())
        {
            std::string results = "Please specify name of player by ?name=ksenia: ";
            session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
            return;
        }
        std::string gender = request->get_query_parameter("gender");
        std::string email = request->get_query_parameter("email");
        std::string email_dum = request->get_query_parameter("laksfj");

        // for (int i = 0; i < 1000; ++i)
        // {
        //     if (email[i])
        //     {
        //     }
        // }

        session->fetch(content_length, [gender, email, name](const shared_ptr<Session> session, const Bytes &body)
                       {
                           //    try
                           //    {
                           //    fprintf(stdout, "Body size: %d\n", (int)body.size());
                           //    fprintf(stdout, "%.*s\n", static_cast<int>(body.size()), body.data());
                           g_db->add_note(name, gender, email, body.data(), body.size());
                           std::string return_body = "OK";
                           session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}});
                           //    }
                           //    catch (std::exception &err)
                           //    {
                           //        std::string results = "Unexpected error happened: ";
                           //        results.append(err.what());
                           //        session->close(INTERNAL_SERVER_ERROR, results, {{"Content-Length", to_string(results.size())}});
                           //    }
                       });
    }
    catch (std::exception &err)
    {
        std::string results = "Unexpected error happened: ";
        results.append(err.what());
        session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
    }
}

void get_pl_info_method_handler(const shared_ptr<Session> session)
{
    const auto request = session->get_request();

    try
    {
        int content_length = request->get_header("Content-Length", 0);
        auto name = request->get_query_parameter("name");
        if (name.empty())
        {
            std::string results = "Please specify name of player by ?name=ksenia: ";
            session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
            return;
        }
        auto gender = request->get_query_parameter("gender");
        auto email = request->get_query_parameter("email");

        session->fetch(content_length, [&](const shared_ptr<Session> session, const Bytes &body)
                       {
                           try
                           {
                                auto result = g_db->search_note(name);
                                std::string return_body ;
                                if (!result.has_value()) {
                                    return_body = "player not found";
                                } else {
                                    char fmt_data[1024];
                                    std::size_t fmt_line_sz = std::sprintf(fmt_data, "{\"name\": %s, \"gender\": %s, \"email\": %s} \n", name.data(), result->first.data(), result->second.data());
                                    return_body = std::string(fmt_data, fmt_data + fmt_line_sz);
                                }
                               session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}});
                           }
                           catch (std::exception &err)
                           {
                               std::string results = "Unexpected error happened: ";
                               results.append(err.what());
                               session->close(INTERNAL_SERVER_ERROR, results, {{"Content-Length", to_string(results.size())}});
                           } });
    }
    catch (std::exception &err)
    {
        std::string results = "Unexpected error happened: ";
        results.append(err.what());
        session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
    }
}

void update_pl_info_method_handler(const shared_ptr<Session> session)
{
    const auto request = session->get_request();

    try
    {
        int content_length = request->get_header("Content-Length", 0);
        auto name = request->get_query_parameter("name");
        if (name.empty())
        {
            std::string results = "Please specify name of player by ?name=ksenia: ";
            session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
            return;
        }
        auto gender = request->get_query_parameter("gender");
        std::string email = request->get_query_parameter("email");

        session->fetch(content_length, [&](const shared_ptr<Session> session, const Bytes &body)
                       {
                           try
                           {
                                g_db->update_note(name, gender, email);
                               std::string return_body = "OK";
                               session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}});
                           }
                           catch (std::exception &err)
                           {
                               std::string results = "Unexpected error happened: ";
                               results.append(err.what());
                               session->close(INTERNAL_SERVER_ERROR, results, {{"Content-Length", to_string(results.size())}});
                           } });
    }
    catch (std::exception &err)
    {
        std::string results = "Unexpected error happened: ";
        results.append(err.what());
        session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
    }
}

void delete_pl_info_method_handler(const shared_ptr<Session> session)
{
    const auto request = session->get_request();

    try
    {
        int content_length = request->get_header("Content-Length", 0);
        auto name = request->get_query_parameter("name");
        if (name.empty())
        {
            std::string results = "Please specify name of player by ?name=ksenia: ";
            session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
            return;
        }

        session->fetch(content_length, [&](const shared_ptr<Session> session, const Bytes &body)
                       {
                           try
                           {
                                g_db->delete_note(name);
                               std::string return_body = "OK";
                               session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}});
                           }
                           catch (std::exception &err)
                           {
                               std::string results = "Unexpected error happened: ";
                               results.append(err.what());
                               session->close(INTERNAL_SERVER_ERROR, results, {{"Content-Length", to_string(results.size())}});
                           } });
    }
    catch (std::exception &err)
    {
        std::string results = "Unexpected error happened: ";
        results.append(err.what());
        session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
    }
}

auto split_to_vec(std::string players_list)
{
    std::vector<std::string> result;
    size_t pos = 0;
    while ((pos = players_list.find(',')) != std::string::npos)
    {
        result.push_back(players_list.substr(0, pos));
        players_list.erase(0, pos + 1);
    }
    result.push_back(players_list);
    return result;
}

void get_pl_info_method_handler_multi(const shared_ptr<Session> session)
{
    const auto request = session->get_request();

    try
    {
        int content_length = request->get_header("Content-Length", 0);
        auto names_str = request->get_query_parameter("names");
        if (names_str.empty())
        {
            std::string results = "Please specify name of player by ?name=ksenia: ";
            session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
            return;
        }
        auto names = split_to_vec(names_str);

        session->fetch(content_length, [&](const shared_ptr<Session> session, const Bytes &body)
                       {
                           try
                           {
                                std::string return_body ;
                                for (auto& name: names) {
                                auto result = g_db->search_note(name);
                                    if (!result.has_value()) {
                                        return_body.append(std::string("player not found: ")).append(name).append("\n");
                                    } else {
                                        char fmt_data[1024];
                                        std::size_t fmt_line_sz = std::sprintf(fmt_data, "{\"name\": %s, \"gender\": %s, \"email\": %s} \n", name.data(), result->first.data(), result->second.data());
                                        return_body.append(std::string(fmt_data, fmt_data + fmt_line_sz));
                                    }
                                }
                               session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}});
                           }
                           catch (std::exception &err)
                           {
                               std::string results = "Unexpected error happened: ";
                               results.append(err.what());
                               session->close(INTERNAL_SERVER_ERROR, results, {{"Content-Length", to_string(results.size())}});
                           } });
    }
    catch (std::exception &err)
    {
        std::string results = "Unexpected error happened: ";
        results.append(err.what());
        session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
    }
}

void get_wait_pdf_method_handler(const shared_ptr<Session> session)
{
    const auto request = session->get_request();

    try
    {
        int content_length = request->get_header("Content-Length", 0);
        auto name = request->get_query_parameter("name");
        if (name.empty())
        {
            std::string results = "Please specify name of player by ?name=ksenia: ";
            session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
            return;
        }

        session->fetch(content_length, [&](const shared_ptr<Session> session, const Bytes &body)
                       {
                           try
                           {

                                if (!g_pdf_creator->check_is_ready(name)) {
                                    std::string return_body = "Not ready yet";
                                    session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}});
                                    return;
                                }
                                
                                std::ifstream ifs(std::string("/tmp/").append(name).append(".pdf"));
                                std::stringstream buffer(std::stringstream::binary | std::stringstream::in | std::stringstream::out);
                                buffer << ifs.rdbuf();
                                std::string return_body = buffer.str();

                                
                               session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}, {"Content-Type", "application/pdf"},
                                    {"Content-Transfer-Encoding", "binary"}});
                               
                           }
                           catch (std::exception &err)
                           {
                               std::string results = "Unexpected error happened: ";
                               results.append(err.what());
                               session->close(INTERNAL_SERVER_ERROR, results, {{"Content-Length", to_string(results.size())}});
                           } });
    }
    catch (std::exception &err)
    {
        std::string results = "Unexpected error happened: ";
        results.append(err.what());
        session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
    }
}

void get_pdf_method_handler(const shared_ptr<Session> session)
{
    const auto request = session->get_request();

    try
    {
        int content_length = request->get_header("Content-Length", 0);
        auto name = request->get_query_parameter("name");
        if (name.empty())
        {
            std::string results = "Please specify name of player by ?name=ksenia: ";
            session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
            return;
        }

        session->fetch(content_length, [&](const shared_ptr<Session> session, const Bytes &body)
                       {
                           
                           try
                           {
                                g_pdf_creator->push(name);

                                std::string return_body = std::string("http://localhost:").append(std::to_string(g_port)).append("/wait_info?name=");
                                return_body.append(name);
                                session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}});
                           }
                           catch (std::exception &err)
                           {
                               std::string results = "Unexpected error happened: ";
                               results.append(err.what());
                               session->close(INTERNAL_SERVER_ERROR, results, {{"Content-Length", to_string(results.size())}});
                           } });
    }
    catch (std::exception &err)
    {
        std::string results = "Unexpected error happened: ";
        results.append(err.what());
        session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
    }
}

void update_hystory_method_handler(const shared_ptr<Session> session)
{
    const auto request = session->get_request();

    try
    {
        int content_length = request->get_header("Content-Length", 0);
        auto name = request->get_query_parameter("name");
        if (name.empty())
        {
            std::string results = "Please specify name of player by ?name=ksenia: ";
            session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
            return;
        }
        auto delta_session_cnt = request->get_query_parameter("delta_session_cnt", 0);
        auto delta_win_cnt = request->get_query_parameter("delta_win_cnt", 0);
        auto delta_lose_cnt = request->get_query_parameter("delta_lose_cnt", 0);
        auto delta_time_sec = request->get_query_parameter("delta_time_sec", 0);

        session->fetch(content_length, [&](const shared_ptr<Session> session, const Bytes &body)
                       {
                           try
                           {
                                g_db->update_history(name, delta_session_cnt, delta_win_cnt,  delta_lose_cnt,  delta_time_sec);
                               std::string return_body = "OK";
                               session->close(OK, return_body, {{"Content-Length", to_string(return_body.size())}});
                           }
                           catch (std::exception &err)
                           {
                               std::string results = "Unexpected error happened: ";
                               results.append(err.what());
                               session->close(INTERNAL_SERVER_ERROR, results, {{"Content-Length", to_string(results.size())}});
                           } });
    }
    catch (std::exception &err)
    {
        std::string results = "Unexpected error happened: ";
        results.append(err.what());
        session->close(BAD_REQUEST, results, {{"Content-Length", to_string(results.size())}});
    }
}

int main(const int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./proxy_server port" << std::endl;
        return 1;
    }
    g_db = std::make_unique<DBClient>("profile.db3");
    g_pdf_creator = std::make_unique<PDFCreator>(g_db.get());

    auto resource = make_shared<Resource>();
    resource->set_path("/player");
    resource->set_method_handler("POST", post_pl_info_method_handler);
    resource->set_method_handler("GET", get_pl_info_method_handler);
    resource->set_method_handler("UPDATE", update_pl_info_method_handler);
    resource->set_method_handler("DELETE", delete_pl_info_method_handler);

    auto resource_hist = make_shared<Resource>();
    resource_hist->set_path("/update_hystory");
    resource_hist->set_method_handler("POST", update_hystory_method_handler);

    auto resource_multi = make_shared<Resource>();
    resource_multi->set_path("/player_list");
    resource_multi->set_method_handler("GET", get_pl_info_method_handler_multi);

    // get_pdf_method_handler
    auto resource_pdf = make_shared<Resource>();
    resource_pdf->set_path("/get_info");
    resource_pdf->set_method_handler("GET", get_pdf_method_handler);

    auto resource_pdf_wait = make_shared<Resource>();
    resource_pdf_wait->set_path("/wait_info");
    resource_pdf_wait->set_method_handler("GET", get_wait_pdf_method_handler);

    auto settings = make_shared<Settings>();
    g_port = atoi(argv[1]);
    settings->set_port(g_port);
    settings->set_default_header("Connection", "close");
    settings->set_reuse_address(true);

    std::cerr << "Server is running" << std::endl;
    Service service;
    service.publish(resource);
    service.publish(resource_multi);
    service.publish(resource_pdf);
    service.publish(resource_pdf_wait);
    service.publish(resource_hist);
    service.start(settings);

    g_pdf_creator->stop_work();
    g_pdf_creator.release();

    g_db.release();

    return 0;
}