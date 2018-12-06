#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 08 September 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

#include <iostream>
#include <string_view>
#include <filesystem>
#include <csignal>
#include <boost/thread.hpp>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <nng/nng.h>
#include <nng/protocol/pipeline0/pull.h>
#include <nng/protocol/pipeline0/push.h>
#include <tsl/utility/scope_guard.h>
#include "add_torrent.proto.h"
#include "config.h"
#include "torrent_manager.h"
#include "torrenter.h"
#include "vpn.h"

namespace
{
    bool shutdown_flag = false;
}

void
fatal(const char * _func, int _retval)
{
    fprintf(stderr, "%s: %s\n", _func, nng_strerror(_retval));
    exit(1);
}

int
main(int argc, char ** argv)
{
    using namespace std::literals;
    auto const catch_signal{[](int _signal) { shutdown_flag = true; }};
    signal(SIGINT, catch_signal); 
    signal(SIGTERM, catch_signal); 
    signal(SIGHUP, catch_signal); 
    signal(SIGKILL, catch_signal); 
    try
    {
        torrenter::vpn vpn("/etc/openvpn/nyc.ovpn");
        vpn.start();
        // setup vpn
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        // setup torrentor
        // These should be config options
        torrenter::torrenter manager{
            "/media/TonyDisk/shares/USB_Storage/TonyDisk/", "/tmp/TVshows"};
        // listen for messages
        nng_socket sock;
        int rv;
        if ((rv = nng_pull0_open(&sock)) != 0)
        {
            fatal("nng_pull0_open", rv);
        }
        if ((rv = nng_listen(sock, "ipc:///tmp/pipeline.ipc", NULL, 0)) != 0)
        {
            fatal("nng_listen", rv);
        }
        for (;;)
        {
            char * buf = NULL;
            size_t sz;
            //if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
            //{
            //    fatal("nng_recv", rv);
            //}
            while (0 != (rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK)) &&
                   !shutdown_flag)
            {
                if (rv == NNG_EAGAIN)
                {
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
                }
                else
                {
                    fatal("nng_recv", rv);
                }
            }
            if (shutdown_flag)
            {
                break;
            }
            // do i break strict aliasing right here? probably
            kj::ArrayPtr<capnp::word> words(
                reinterpret_cast<capnp::word *>(buf), sz / sizeof(capnp::word));
            capnp::FlatArrayMessageReader message(words);
            auto reader(message.getRoot<BaseMessage>());
            std::cout << "received message:" << reader.toString().flatten().begin() << std::endl;
            auto data = reader.getData();
            switch (data.which())
            {
                case BaseMessage::Data::EMPTY:
                {
                    std::cout << "received empty message" << std::endl;
                }
                break;
                case BaseMessage::Data::ADD_TORRENT:
                {
                    std::cout << "received add torrent message" << std::endl;
                    manager.handle_add_torrent(data.getAddTorrent());
                }
                break;
                default:
                {
                    std::cout << "received unknown message" << std::endl;
                }
            }
            nng_free(buf, sz);
        }
    }
    catch(std::exception & _exception)
    {
        std::cout << "Caught exception, exiting: " << _exception.what() << std::endl;
    }
    return 0;
}
