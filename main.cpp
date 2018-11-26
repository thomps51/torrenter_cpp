#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 08 September 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

#include <stdexcept>
#include <iostream>
#include <string_view>
#include <filesystem>
#include <regex>
#include <boost/process.hpp>
#include <boost/thread.hpp>
#include "config.h"
#include "torrent_manager.h"

#include <nng/nng.h>
#include <nng/protocol/pipeline0/pull.h>
#include <nng/protocol/pipeline0/push.h>
#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include "add_torrent.proto.h"

#include "torrenter.h"

void
fatal(const char * func, int rv)
{
    fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
    exit(1);
}


void
vpn_thread()
{
    auto start = std::chrono::system_clock::now();
    bool first(true);
    for (;;)
    {
        //int const result(
        //    boost::process::system("ip netns exec piavpn openvpn --config /etc/openvpn/nyc.ovpn",
        //        boost::process::std_out > "piavpn.out", boost::process::std_err > "piavpn.err"));
        if (!first && std::chrono::system_clock::now() - start < std::chrono::seconds(5))
        {
            std::cerr << "openvpn failed within 5 seconds of previous failure: check logs"
                      << std::endl;
            std::exit(EXIT_FAILURE);
        }
        first = false;
        start = std::chrono::system_clock::now();
        int const result(
            boost::process::system("openvpn --config /etc/openvpn/nyc.ovpn",
                boost::process::std_out > "piavpn.out", boost::process::std_err > "piavpn.err"));
        std::cout << "VPN process has exited, retrying after 1 second" << std::endl;
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
    }
}

int
main(int argc, char ** argv)
{
    using namespace std::literals;
    //try
    //{
        // setup vpn
        auto vpn_watcher{boost::thread(vpn_thread)}; 
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
            if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC)) != 0)
            {
                fatal("nng_recv", rv);
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
        vpn_watcher.join();
    //}
    //catch(...)
    //{
    //    std::cout << "Caught exception, exiting" << std::endl;
    //}
    return 0;
}
