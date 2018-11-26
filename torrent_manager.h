#pragma once
#ifndef included_torrenter_torrent_manager_09September2018_
#define included_torrenter_torrent_manager_09September2018_

#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 09 September 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

//! @file
//! Provides torrenter::torrent_manager

#include <vector>
#include <boost/chrono.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>
#include <filesystem>

namespace torrenter
{
    class torrent_manager
    {
    public:
        torrent_manager(std::filesystem::path const & _temp_dir);

        void
        add(std::string_view const & _magnet_url,
            std::function<void(boost::shared_ptr<libtorrent::torrent_info const> _info)>
                _callback);

    private:
        boost::chrono::hours max_seed_time_;
        size_t max_seed_ratio_;
        libtorrent::session session_;
        std::string temp_dir_;
        std::vector<boost::thread> torrent_threads_; // this does not get cleaned up yet

        void
        thread_proc(libtorrent::torrent_handle _handle,
            std::function<void(boost::shared_ptr<libtorrent::torrent_info const> _info)> _callback);
    };
}
#endif
