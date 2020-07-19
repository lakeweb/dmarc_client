// project.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "project.h"

int main()
{
    session_params params = { 110,"lakeweb.net","hop","eboard" };
    pop3_session(params);
    //pop3_test();
    {//one file test
 /*       bfs::path path("./testing/test11.txt");
        auto result = process_email(read_file(path.c_str()));
        std::cout << result;
 */   }
    return 0;

}

