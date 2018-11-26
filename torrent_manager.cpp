#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 09 September 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

#include "torrent_manager.h"
#include <filesystem>
#include <libtorrent/torrent_status.hpp>

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
        if (!std::filesystem::exists(temp_dir_))
        {
            std::filesystem::create_directory(temp_dir_);
        }
    }

    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    torrent_manager::add(std::string_view const & _magnet_url,
        std::function<void(boost::shared_ptr<libtorrent::torrent_info const> _info)> _callback)
    {
        libtorrent::add_torrent_params params;
        params.url = _magnet_url;
        params.save_path = temp_dir_;
        auto handle(session_.add_torrent(params));
        torrent_threads_.emplace_back(
            boost::thread(&torrent_manager::thread_proc, this, handle, _callback));
    }

    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    torrent_manager::thread_proc(libtorrent::torrent_handle _handle,
        std::function<void(boost::shared_ptr<libtorrent::torrent_info const> _info)>
            _callback)
    {
        std::cout << "starting torrent" << std::endl;
        while (! _handle.has_metadata())
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        }
        std::cout << "got metadata" << std::endl;
        while (_handle.status().state != libtorrent::torrent_status::seeding)
        {
            //auto status(_handle.status());
            //std::cout << "\r" << (status.download_payload_rate / 1000) << " kB/s "
            //          << (status.total_done / 1000) << " kB (" << (status.progress_ppm / 10000)
            //          << "%) downloaded\x1b[K";
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        }
        boost::chrono::system_clock::time_point seed_start_time(boost::chrono::system_clock::now());
        _callback(_handle.torrent_file());
        //std::cout << "max seed ratio: " << max_seed_ratio_ << std::endl;
        //std::cout << "total download: " << _handle.status().total_download << std::endl;
        //std::cout << "total upload: " << _handle.status().total_upload << std::endl;
        while (max_seed_ratio_ * _handle.status().total_download > _handle.status().total_upload)
        {
            //std::cout << "seeding..." << std::endl;
            if (seed_start_time + max_seed_time_ < boost::chrono::system_clock::now())
            {
                std::cout << "seeding timed out" << std::endl;
                break;
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));
            //std::cout << "max seed ratio: " << max_seed_ratio_ << std::endl;
            //std::cout << "total download: " << _handle.status().total_download << std::endl;
            //std::cout << "total upload: " << _handle.status().total_upload << std::endl;
        }
        std::cout << "removing torrent" << std::endl;
        session_.remove_torrent(_handle, libtorrent::session::delete_files);
        std::cout << "removing data" << std::endl;
        // remove temp data
    }
}
