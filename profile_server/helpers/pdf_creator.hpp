#pragma once
#include <thread>
#include <queue>
#include <string>
#include <memory>
#include <condition_variable>
#include <chrono>
#include <map>
#include <iostream>
#include "../libharu/include/hpdf.h"
#include "db_client.hpp"

using namespace std::chrono_literals;
void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    std::cerr << "error! " << error_no << " " << detail_no << '\n';
    throw std::exception();
}
class PDFCreator
{

public:
    PDFCreator(DBClient *db_client_) : db_client(db_client_)
    {
        worker_ = std::make_unique<std::thread>([this]()
                                                {
            while(this->is_work.load()) {
                std::unique_lock lock(mutex_);
                if (!task_queue_.empty()) {
                    std::string next_name = task_queue_.front();
                    std::cerr << "start complete new task" << std::endl;
                    task_queue_.pop();
                    lock.unlock();
                    // std::this_thread::sleep_for(5s); // for homework checking


                    HPDF_Doc pdf;

                    pdf = HPDF_New (error_handler, NULL);
                    if (!pdf) {
                        printf ("ERROR: cannot create pdf object.\n");
                        return 1;
                    }
                    HPDF_Page page_1;

                    page_1 = HPDF_AddPage (pdf);
                    auto def_font = HPDF_GetFont (pdf, "Times-Roman", NULL);
                    // auto font_name = HPDF_LoadType1FontFromFile (pdf, "../libharu/demo/type1/a010013l.afm",  "../libharu/demo/type1/a010013l.pfb");
                    // auto def_font = HPDF_GetFont (pdf, font_name, "KOI8-R");
                    HPDF_Page_SetFontAndSize (page_1, def_font, 24);
                    int height = HPDF_Page_GetHeight (page_1);
                    int width = HPDF_Page_GetWidth (page_1);
                    HPDF_Page_BeginText(page_1);
                    auto all_info = db_client->get_complete_info(next_name);
                    int ind = 0;
                    for (auto& [wh, val]: all_info) {
                        ++ind;
                        std::string next_line =  std::string(wh).append(val).c_str();
                        HPDF_Page_TextOut (page_1, 50, height - 50 * ind, next_line.c_str());
                    }
                    HPDF_Page_EndText(page_1);

                    HPDF_SaveToFile (pdf, std::string("/tmp/").append(next_name).append(".pdf").c_str());
                    is_ready[next_name].store(true);

                    lock.lock();
                }
                this->wait_finish_work_.wait(lock, [this]() {return (is_work.load() == false) || (!task_queue_.empty());});

            } });
    }

    void push(std::string new_name)
    {
        {
            std::lock_guard lock(mutex_);
            if (is_ready.find(new_name) != is_ready.end())
            {
                return;
            }
            is_ready[new_name].store(false);
            task_queue_.push(std::move(new_name));
        }
        wait_finish_work_.notify_one();
    }

    bool check_is_ready(const std::string &new_name)
    {
        return is_ready[new_name].load();
    }

    void stop_work()
    {
        is_work.store(false);
        wait_finish_work_.notify_one();
        worker_->join();
        worker_.release();
    }

private:
    std::map<std::string, std::atomic_bool> is_ready;
    std::unique_ptr<std::thread> worker_;
    std::queue<std::string> task_queue_;
    std::mutex mutex_;
    std::atomic_bool is_work{true};
    std::condition_variable wait_finish_work_;

    DBClient *db_client;
};