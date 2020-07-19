
#pragma once

//a session socket..........
struct socket_session {
    boost::asio::io_service io;
    boost::asio::ip::tcp::socket socket;

    socket_session(const std::string& server, size_t port) :socket(io) {
        boost::asio::ip::tcp::resolver resolver(io);
        boost::asio::ip::tcp::resolver::query query(server, std::to_string(port));
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::asio::connect(socket, endpoint_iterator);
    }
    std::string buffer_to_string(const boost::asio::streambuf& buffer)
    {
        using boost::asio::buffers_begin;
        auto bufs = buffer.data();
        std::string result(buffers_begin(bufs), buffers_begin(bufs) + buffer.size());
        return result;
    }
    std::string get_response(size_t buf_size = 2000) {
        boost::asio::streambuf response(buf_size);
        boost::asio::read_until(socket, response, "\n");
        return buffer_to_string(response);
    }
    std::string read_port(size_t size) {
        boost::asio::streambuf response;
        ////        boost::asio::read_so
        boost::system::error_code err;
        boost::asio::read(socket, response, boost::asio::transfer_exactly(size), err);
        return buffer_to_string(response);
    }
    std::string read_until(const char* until) {
        boost::asio::streambuf response(512);
        boost::asio::read_until(socket, response, "\r\n");
        return buffer_to_string(response);
    }
    std::string read_all() {
        boost::asio::streambuf response;
        ////        boost::asio::read_so
        boost::system::error_code err;
        boost::asio::read(socket, response, boost::asio::transfer_all(), err);
        return buffer_to_string(response);
    }
    void query(std::string str) {
        boost::asio::write(socket, boost::asio::buffer(str + '\n'));
    }
    boost::asio::ip::tcp::socket* get_socket() { return &socket; }
};

//our types............
//using mail_item_type = std::tuple<size_t, size_t>;
using mail_item_type = std::pair<size_t, size_t>;
using mail_set_type = std::vector<mail_item_type>;
using retarg_type = std::optional<std::string>;
using string_iter = std::string::const_iterator;
using string_range = std::pair<string_iter, string_iter>;
using string_range_set = std::vector<string_range>;

//port parameters
struct session_params {
    size_t port;
    std::string site;
    std::string user;
    std::string password;
};

struct edata_type {
    std::string subject;
    std::string date;
    std::string text_data;
    std::string xml_data;
    std::string error;
    //
    std::string zip;
};
//BOOST_FUSION_ADAPT_STRUCT(edata_type,subject,date)

//Media types so far encountered for dmarc notifications
namespace media_types {
    static std::vector<std::string_view> content_lable = {
        {"multipart/mixed"},
        {"text/plain"},
        {"application/gzip"},
        {"application/zip"}
    };
    enum content_id {
        multipart,
        text,
        gzip,
        zip,
        unknown = -1,
    };
    inline content_id lookup_type(const std::string& in) {
        auto found = std::find_if(content_lable.begin(), content_lable.end(),
            [in](const std::string_view& v) {return in == v; });
        if (found == content_lable.end())
            return unknown;
        return (content_id)(found - content_lable.begin());
    }
}//namespace

//parsers...............
namespace parsers {
    using namespace boost::spirit::x3;
    const auto skipto_eol = lexeme[omit[*(char_ - eol)]];
    const auto parseto_eol = lexeme[*(char_ - eol)];
    const auto word_mix = +char_("a-zA-Z0-9/:");
    const auto parse_content = seek["Content-Type: "] >> *char_("a-z/") >> -seek["\r\n\r\n"];
    static const auto parse_mail_item = rule<class _, mail_item_type>("mail_item_type") = int_ >> int_;
}

//zlib ...............
#include <c:/cpp/zlib/zlib.h>
struct zas_file {
    std::string filename;
    std::string comments;
    std::string data;
};
#define WBITS 15

//from utils.cpp
zas_file za_get_firstfile(char* zd);
void get_content(string_iter begin, string_iter end, edata_type& edata, const media_types::content_id id);
edata_type process_email(const std::string& mail);
retarg_type pop3_session(const session_params& params);
std::ostream& operator << (std::ostream& os, edata_type& ed);
void pop3_test();

