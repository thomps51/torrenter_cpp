#pragma once
#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 04 September 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

#include <string>
#include <string_view>
#include "add_torrent.proto.h"
#include "torrent_manager.h"

namespace torrenter
{
    /*
     */
    class torrenter
    {
    public:
        torrenter(std::string_view const & _main_dir, std::string_view const & _temp_dir);

        void
        start();

        void
        stop();

        void
        handle_add_torrent(AddTorrent::Reader const & _reader);

    private:
        std::string                  main_dir_;
        std::string                  temp_dir_;
        ::torrenter::torrent_manager manager_;
    };
}
