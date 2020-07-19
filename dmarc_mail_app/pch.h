#pragma once
#include <iostream>
#include <fstream>
#include <optional>

#include <zlib.h>

#include <boost/asio.hpp>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>

//b2.exe --with-iostreams -s BZIP2_SOURCE=C:\cpp\bzip2-1.0.6 -s ZLIB_SOURCE=C:\cpp\zlib-1.2.11
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
namespace bio = boost::iostreams;

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
namespace bai = boost::archive::iterators;

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

