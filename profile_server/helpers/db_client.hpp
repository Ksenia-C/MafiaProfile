#pragma once
#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include <optional>
#include <tuple>

class DBClient
{
public:
    DBClient(std::string db_name) : db(db_name, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
    {

        SQLite::Statement query(db, "CREATE TABLE IF NOT EXISTS Profile("
                                    "NAME           TEXT    NOT NULL,"
                                    "GENDER         TEXT     ,"
                                    "EMAIL          TEXT,"
                                    "PHOTO_SZ          INTEGER,"
                                    "PHOTO          BLOB );");

        query.exec();

        query = SQLite::Statement(db, "CREATE TABLE IF NOT EXISTS SessionHistory("
                                      "NAME           TEXT    NOT NULL,"
                                      "session_cnt       INTEGER,"
                                      "win_cnt           INTEGER     ,"
                                      "lose_cnt          INTEGER,"
                                      "time_sec          INTEGER);");

        query.exec();
    }

    void update_history(int delta_session_cnt, int delta_win_cnt, int delta_lose_cnt, int delta_time_sec)
    {

        SQLite::Statement query(db, "UPDATE SessionHistory SET session_cnt = session_cnt + ?, win_cnt = win_cnt + ?, lose_cnt = lose_cnt + ?, time_sec = time_sec + ? "
                                    "WHERE NAME = ?;");
        query.bind(1, delta_session_cnt);
        query.bind(2, delta_win_cnt);
        query.bind(3, delta_lose_cnt);
        query.bind(4, delta_time_sec);
        query.exec();
    }

    void add_note(const std::string &name, const std::string &gender, const std::string &email, const void *imag, int im_sz)
    {

        SQLite::Statement query(db, "INSERT INTO Profile (NAME,GENDER,EMAIL) "
                                    "SELECT ?, ?, ? "
                                    "WHERE NOT EXISTS(SELECT 1 FROM Profile WHERE NAME = ?);");
        query.bind(1, name);
        query.bind(2, gender);
        query.bind(3, email);
        // query.bind(4, im_sz);
        // query.bind(5, imag, im_sz);
        query.bind(4, name);
        query.exec();

        query = SQLite::Statement(db, "INSERT INTO SessionHistory (session_cnt,win_cnt,lose_cnt, time_sec) "
                                      "SELECT 0, 0, 0, 0 "
                                      "WHERE NOT EXISTS(SELECT 1 FROM Profile WHERE NAME = ?);");
        query.bind(1, name);
        query.exec();
    }

    std::optional<std::pair<std::string, std::string>> search_note(const std::string &name)
    {

        SQLite::Statement query(db, "SELECT GENDER, EMAIL FROM Profile WHERE NAME = ?;");
        query.bind(1, name);
        while (query.executeStep())
        {
            const char *gender = query.getColumn(0);
            const char *email = query.getColumn(1);
            // int phot_len = query.getColumn(2);
            // const void *photo = query.getColumn(3).getBlob();

            // std::string photo_bin('0', phot_len);
            // strncpy(photo_bin.data(), (char *)photo, phot_len);

            return std::make_pair(std::string(gender), std::string(email));
        }
        return std::nullopt;
    }

    void delete_note(const std::string &name)
    {

        SQLite::Statement query(db, "DELETE FROM Profile WHERE NAME = ?;");
        query.bind(1, name);
        query.exec();
    }

    void update_note(const std::string &name, const std::string &gender, const std::string &email)
    {
        if (!gender.empty())
        {
            SQLite::Statement query(db, "UPDATE Profile SET GENDER = ? WHERE NAME = ?;");
            query.bind(1, gender);
            query.bind(2, name);
            query.exec();
        }
        if (!email.empty())
        {
            SQLite::Statement query(db, "UPDATE Profile SET EMAIL = ? WHERE NAME = ?;");
            query.bind(1, email);
            query.bind(2, name);
            query.exec();
        }
    }

    std::vector<std::pair<std::string, std::string>> get_complete_info(const std::string &name)
    {
        std::vector<std::pair<std::string, std::string>> result;
        result.push_back({"User name: ", name});
        auto profile_info = search_note(name);
        if (!profile_info.has_value())
        {
            result.push_back({"Is registred: ", "no"});
            return result;
        }
        result.push_back({"Gender: ", profile_info->first});
        result.push_back({"Email: ", profile_info->second});

        SQLite::Statement query(db, "SELECT session_cnt,win_cnt,lose_cnt, time_sec FROM SessionHistory WHERE NAME = ?;");
        query.bind(1, name);
        while (query.executeStep())
        {
            result.push_back({"Amount of played sessions: ", std::to_string(query.getColumn(0).getInt())});
            result.push_back({"Amount of victories: ", std::to_string(query.getColumn(1).getInt())});
            result.push_back({"Amount of defeats: ", std::to_string(query.getColumn(2).getInt())});
            result.push_back({"Total time in game (sec): ", std::to_string(query.getColumn(3).getInt())});
            break;
        }
        return result;
    }

private:
    SQLite::Database db;
};