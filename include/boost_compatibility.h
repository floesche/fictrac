#pragma once
#include <boost/version.hpp>
#include <boost/asio.hpp>
// Starting with boost 1.66.0, some function names have been changed.
#if BOOST_VERSION >= 106600
    using asio_io = boost::asio::io_context;
    #define ASIO_MAKE_ADDRESS boost::asio::ip::make_address
#else
    using asio_io = boost::asio::io_service;
    #define ASIO_MAKE_ADDRESS boost::asio::ip::address::from_string
#endif