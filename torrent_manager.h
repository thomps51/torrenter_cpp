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
        using callback_type =
            std::function<void(boost::shared_ptr<libtorrent::torrent_info const>)>;
        
        torrent_manager(std::filesystem::path const & _temp_dir);

        void
        add(std::string_view const & _magnet_url, callback_type const & _callback);

    private:
        size_t                     max_seed_ratio_;
        boost::chrono::hours       max_seed_time_;
        libtorrent::session        session_;
        std::string                temp_dir_;

        struct torrent_info
        {
            callback_type                           callback;
            std::optional<boost::chrono::hours>     max_seed_time;
            std::optional<unsigned>                 seeding_ratio;
            boost::chrono::system_clock::time_point seed_start_time;
        };

        std::unordered_map<boost::uint32_t, torrent_info> torrents_; 
    
        void
        main_thread();
    };
}
#endif
