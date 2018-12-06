#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 09 September 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

#include "torrent_manager.h"
#include <libtorrent/extensions/smart_ban.hpp>
#include <libtorrent/torrent_status.hpp>
#include <tsl/utility/preprocessor.h>

namespace torrenter
{
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    torrent_manager::torrent_manager(std::filesystem::path const & _temp_dir)
        : max_seed_ratio_(3u)
        , max_seed_time_(boost::chrono::hours(240))
        , temp_dir_(_temp_dir)
    {
        session_.add_extension(&libtorrent::create_smart_ban_plugin);
        if (!std::filesystem::exists(temp_dir_))
        {
            std::filesystem::create_directory(temp_dir_);
        }
    }

    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    torrent_manager::add(std::string_view const & _magnet_url, callback_type const & _callback)
    {
        libtorrent::add_torrent_params params;
        params.url = _magnet_url;
        params.save_path = temp_dir_;
        std::cout << "adding torrent" << _magnet_url << std::endl;
        auto handle{session_.add_torrent(params)};
        auto & info{torrents_[handle.id()]};
        info.callback = _callback;
        info.max_seed_time = max_seed_time_;
        info.seeding_ratio = max_seed_ratio_;
    }

    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    torrent_manager::main_thread()
    {
        TSL_INFINITE_LOOP_BEGIN
        auto const torrents{session_.get_torrents()};
        for (auto const & torrent : torrents)
        {
            auto const & status{torrent.status()};
            if (status.state != libtorrent::torrent_status::seeding)
            {
                continue; // TODO: maybe produce warning about torrents that have been
                          // downloding for a long time.
            }
            auto itr = torrents_.find(torrent.id());
            if (itr == torrents_.end())
            {
                // makes no sense bro
                continue;
            }
            auto & info{itr->second};
            if (info.callback)
            {
                info.callback(torrent.torrent_file());
                info.callback = nullptr;
            }
            if (!info.seed_start_time.time_since_epoch().count())
            {
                info.seed_start_time = boost::chrono::system_clock::now();
            }
            bool const amount_reached(
                info.seeding_ratio &&
                *info.seeding_ratio * status.total_download < status.total_upload);
            bool const time_reached(
                info.max_seed_time && info.seed_start_time + *info.max_seed_time >
                                          boost::chrono::system_clock::now());
            bool const done_uploading(amount_reached && time_reached);
            if (done_uploading)
            {
                std::cout << "removing torrent " << status.name << std::endl;
                session_.remove_torrent(torrent, libtorrent::session::delete_files);
            }
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(5000));
        TSL_INFINITE_LOOP_END
    }
}
