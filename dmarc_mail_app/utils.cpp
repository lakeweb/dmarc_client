
#include "pch.h"
#include "project.h"

// .........................................
std::string read_file(bfs::path filepathname)
{
    std::ifstream is(filepathname.wstring(), std::ios::binary | std::ios::ate);
    auto buf_size = is.tellg();
    if (buf_size == -1)
        return std::string();

    std::string str;
    str.resize((size_t)buf_size);
    is.seekg(0, std::ios::beg);
    is.read(str.data(), str.size());
    return str;
}

// .........................................
auto decode_base64(string_iter begin, string_iter end) {
    using it_binary_t = bai::transform_width< bai::binary_from_base64<bai::remove_whitespace<std::string::const_iterator> >, 8, 6 >;
    for (; *end == '.' || *end == '\n' || *end == '\r'; --end)//not all emails are consitant
        ;
    ++end;
    std::string buf(begin, end);
    unsigned int paddChars = std::count(begin, end, '=');
    // replace '=' by base64 encoding of '\0'
    std::replace(buf.begin(), buf.end(), '=', 'A');
    // decode
    std::string result;
    result.assign(it_binary_t(buf.begin()), it_binary_t(buf.end()));
    result.erase(result.end() - paddChars, result.end());
    std::ofstream testzip("./testing/testzip.txt", std::ios::binary);
    testzip << result;
    return std::istringstream(result, std::ios_base::in | std::ios_base::binary);
}

// .........................................
zas_file za_get_firstfile(char* zd) {
    z_stream_s zs;
    memset(&zs, 0, sizeof(z_stream));
    inflateInit2(&zs, -WBITS);

    zs.avail_in = *(uint32_t*)(zd + 18);
    zs.avail_out = *(uint32_t*)(zd + 22);

    zd += 26;

    zas_file out;
    std::string filename(zd + 4, *(uint16_t*)zd);
    out.filename.assign(zd + 4, *(uint16_t*)zd);
    zd += *(uint16_t*)zd + *(uint16_t*)(zd + 2) + 4;

    out.data.resize(zs.avail_out);

    zs.next_in = (Bytef*)zd;
    zs.next_out = (Bytef*)out.data.data();

    auto err = inflate(&zs, Z_SYNC_FLUSH);
    err = inflateEnd(&zs);
    return out;
}

// .........................................
void get_content(string_iter begin, string_iter end, edata_type& edata, const media_types::content_id id) {

    auto get_data_pos = [&end](string_iter begin) {
        auto at = std::find_if(begin, end, [](auto& it) { return !memcmp("\r\n\r\n", &it, 4); });
        if (at != end)
            at += 4;
        return at;
    };

    switch (id) {
    case media_types::multipart:
        //already have the boundry marker
        std::cout << "case: multipart\n";
        return;
    case media_types::text:
        edata.text_data.assign(begin, end);
        std::cout << " case: text" << edata.text_data << "\n";
        break;
    case media_types::gzip:
    {
        auto stream = decode_base64(get_data_pos(begin), end);
        //{
        //    std::string temp(stream.str());
        //    edata.zip.assign(temp.begin(), temp.end());
        //}
        bio::filtering_istreambuf in;
        in.push(bio::gzip_decompressor());
        in.push(stream);
        std::ostringstream os;
        try { bio::copy(in, os); }
        catch (std::exception & e) { std::cout << e.what() << std::endl; }
        edata.xml_data = os.str();
        break;
    }
    case media_types::zip:
    {
        auto stream = decode_base64(get_data_pos(begin), end);
        std::string temp(stream.str());
        edata.xml_data = za_get_firstfile(temp.data()).data;
        edata.zip = temp;
        break;
    }
    }//switch
}

// .........................................
edata_type process_email(const std::string& mail) {

    using namespace parsers;
    edata_type edata;
    auto begin(mail.begin());

    //like this as submitter and date come in ether order
    if (!phrase_parse(begin, mail.end(), seek["Submitter: "] >> lexeme[*(char_ - eol)], space, edata.subject)) {
        edata.error = "failed to find 'Submitter'";
        return edata;
    }
    std::cout << edata.subject << std::endl;
    begin = mail.begin();
    if (!phrase_parse(begin, mail.end(), seek["Date:"] >> parseto_eol, space, edata.date)) {
        edata.error = "failed to find 'Date'";
        return edata;
    }
    //not nessasary to have a boundry if only one Content-Type
    string_range_set boundries;
    std::string boundry;
    if (phrase_parse(begin, mail.end(), seek["multipart/mixed"] >> seek["boundary=\""] >> *(char_ - '"'), space, boundry)) {
        phrase_parse(begin, mail.end(), seek[boundry], space);
        string_iter it = begin;
        for (; ; ) {
            if (parse(begin, mail.end(), seek[boundry]))
                boundries.push_back({ it, begin - boundry.size() - 4 });
            else //eof
                break;
            it = begin + 2;
        }
    }
    //for (auto test : boundries)
    //    std::cout << "**********\n" << std::string(test.first, test.second) << "**********\n";

    begin = mail.begin();
    if (boundries.size()) {
        for (auto an_it : boundries) {
            std::string content;
            if (phrase_parse(an_it.first, mail.end(), parse_content, space, content))
                get_content(an_it.first, an_it.second, edata, media_types::lookup_type(content));
            else break; // and error...
        }
    }
    else {
        begin = mail.begin();
        std::string content;
        if (phrase_parse(begin, mail.end(), parse_content, space, content))
            get_content(begin, mail.end() - 1, edata, media_types::lookup_type(content));
    }
    return edata;
}

// .........................................
retarg_type pop3_session(const session_params& params) {
    //
    socket_session sock(params.site, params.port);
    if (sock.get_response().compare(0, 3, "+OK"))
        assert(false);//failed, deal with it

    //for POP3 commands
    auto post_get_check = [&sock](std::string cmd) ->retarg_type {
        sock.query(cmd);
        std::string resp(sock.read_until("\r\n"));
        if (resp.compare(0, 3, "+OK")) {
            std::cout << "ERROR: " << resp << std::endl;
            assert(false);//deal with it
            return retarg_type();
        }
        std::cout << cmd << " : " << resp.substr(4);
        return resp.substr(4);
    };
    //setup
    retarg_type retstr;
    std::vector<std::string> start_up =
    { {"USER " + params.user},{"PASS " + params.password},{"LIST"} };
    for (auto& item : start_up)
        if (!(retstr = post_get_check(item))) {
            assert(false); //deal with it
            std::cout << *retstr << std::endl;
        }
    //the list
    std::string& list = *retstr;
    list += sock.read_until("\n");
    list += sock.read_until("\n");
    //the size of the LIST
    auto list_size = std::atoi(list.data());

    auto begin(list.begin());
    mail_set_type mails_size;
    {
        using namespace parsers;
        phrase_parse(begin, list.end(), skipto_eol >> *parse_mail_item, space, mails_size);
    }
    assert(list_size == mails_size.size());//another deal with it.

    for (auto& item : mails_size) {
        auto query = std::string("RETR ") + std::to_string(item.first);

        //can't do it this way, hangs as there is more data. why?
        //std::string resp(sock.get_response());
        //if (resp.compare(0, 3, "+OK")) {
        //    assert(false); //uno..
        //}
        sock.query(query);
        auto test_str = sock.read_port(item.second);
        test_str += sock.read_until("\n");

        auto result = process_email(test_str);
        bfs::path test_out("../test_files");//the test folder
        {
            bfs::path path(test_out / (std::string("pop") + std::to_string(item.first) + ".txt"));
            std::ofstream testos(path.c_str(), std::ios::binary);
            testos << result.subject << '\n'
                << result.date << '\n'
                << result.xml_data << '\n'
                << result.error << "\n\n";
            if (result.zip.size()) {
                bfs::path rawpath(test_out / (std::string("zip") + std::to_string(item.first) + ".zip"));
                std::ofstream raw(rawpath.c_str(), std::ios::binary);
                raw << result.zip;
            }
            std::cout << item.first << ' ' << result.error << std::endl;
        }
    }
    retstr = *post_get_check("QUIT");
    std::cout << *retstr;
    return retstr;
}

// .........................................
std::ostream& operator << (std::ostream& os, edata_type& ed) {
    return os << ed.date << '\n'
        << ed.subject << '\n'
        << ed.text_data << '\n'
        << ed.xml_data << '\n';
}

// .........................................
void pop3_test() {
    //using namespace parsers;
    size_t cnt = 0;
    bfs::directory_iterator dir_begin(bfs::path("../testfiles"));
    for (auto it = dir_begin; it != bfs::directory_iterator(); ++it) {
        auto check(it->path().string());
        auto at = std::find_if(check.begin() + 10, check.end(), [](auto& it) { return !memcmp("test", &it, 4); });
        if (at == check.end())
            continue;
        std::cout << "test: " << ++cnt << " " << it->path().string() << std::endl;
        auto result = process_email(read_file(it->path().c_str()));
        std::cout << "  " << result.error << std::endl;
        std::string pstr(it->path().string());
        auto num = std::find_if(pstr.begin(), pstr.end(), [](const auto ch) { return std::isdigit(ch); });
        if (num == pstr.end())
            continue;
        bfs::path test_out("../testfiles");//the test folder
        {
            bfs::path path(test_out / (std::string("content") + &*num));
            std::ofstream testos(path.c_str(), std::ios::binary);
            testos << result.date << '\n'
                << result.subject << '\n'
                << result.text_data << '\n'
                << result.xml_data << '\n';
        }
        bfs::path path(test_out / (std::string("zip") + &*num + ".zip"));
        std::ofstream zfile(path.string(), std::ios::binary);
        zfile.write((char*)result.zip.data(), result.zip.size());
    }
}
