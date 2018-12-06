#pragma once
#ifndef included_torrenter_vpn_05December2018_
#define included_torrenter_vpn_05December2018_

#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 05 December 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

//! @file
//! Provides FUNCTION/CLASS  //TODO

#include <atomic>
#include <filesystem>
#include <boost/thread.hpp>

namespace torrenter
{
    class vpn
    {
    public:
        ~vpn();

        vpn(std::filesystem::path const & _config_file);

        void
        start();

        void
        stop();

    private:
        boost::thread thread_;
        std::atomic<bool> stopped_;

        void
        thread_proc();
    };
}

#endif
