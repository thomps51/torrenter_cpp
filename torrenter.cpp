#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 25 November 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

#include "torrenter.h"
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <iostream>

namespace
{
    std::string_view
    get_download_folder(AddTorrent::Reader const & _reader);

    /*
        Returns the path to the largest file in @c _files.
     */
    std::filesystem::path
    get_largest_file(
        libtorrent::file_storage const & _files, std::filesystem::path const & _temp_dir);
}

namespace torrenter
{
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    torrenter::torrenter(std::string_view const & _main_dir, std::string_view const & _temp_dir)
        : main_dir_(_main_dir)
        , temp_dir_(_temp_dir)
        , manager_(temp_dir_)
    {
        std::cout << "Constructed main_dir:" << main_dir_ << std::endl;

    }

    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    torrenter::handle_add_torrent(AddTorrent::Reader const & _reader)
    {
        std::cout << "Received AddTorrent Message" << std::endl;
        auto download_folder = get_download_folder(_reader);
        auto location = _reader.getLocation();
        std::string_view magnet_url;
        switch (location.which())
        {
            case AddTorrent::Location::MAGNET_URL:
            {
                auto temp = location.getMagnetUrl();
                magnet_url = std::string_view(temp.cStr(), temp.size());
            }
            break;
            default:
            {
                throw std::runtime_error("Only Magnet URLs are supported");
            }
            break;
        }
        auto const options(_reader.getDownloadOptions());
        auto const end(options.end());

        bool found{false};
        std::for_each(options.begin(), options.end(), [&found](auto value) {
            if (value == AddTorrent::DownloadOption::KEEP_ONLY_LARGEST_FILE)
            {
                found = true;
            }
        });
        if (found)
        {
            auto const callback([this, relative_destination = std::string(download_folder)](
                                    boost::shared_ptr<libtorrent::torrent_info const> _info) 
            {
                std::cout << "Download done, in callback" << std::endl;
                using std::filesystem::path;
                std::cout << "Main Dir: " << main_dir_ << std::endl;
                path destination(main_dir_);
                destination /= relative_destination;
                std::cout << "Destination: " << destination.string() << std::endl;
                auto const & files(_info->files());
                auto const source(get_largest_file(files, temp_dir_));
                std::cout << "Source: " << source.string() << std::endl;
                std::filesystem::create_directories(destination);

                std::string dest(destination.string());
                boost::replace_all(dest, "'", "'\"'\"'");

                path file_path = destination / source.filename();
                if (std::filesystem::is_regular_file(file_path))
                {
                    std::cout << "file already exists, skipping copy" << std::endl;
                    return;
                }
                std::string command("cp '" + source.string() + "' '" + dest + "'");
                std::cout << "running copy command: " << command << std::endl;
                // This is stupid from a security standpoint.  I am accepting arbitrary strings from
                // messages and adding them as part of a raw command.  I need to change this,
                // especially when I move away from IPC.  Maybe std::filesystem will work correctly
                // with mounted network drives one day...
                std::lock_guard<std::mutex> _(system_mutex_); // don't flood the IO
                system(command.c_str());
                //std::string permissions("chmod -R 777 " + destination.string());
                // this does not work with mounted network filesysem
                // std::filesystem::copy(source, destination);
                std::cout << "copy done" << std::endl;
            });
            manager_.add(magnet_url, callback);
        }
        else
        {
            throw std::runtime_error("Not Implemented");
            // don't only get the largest file
        }
    }
}

namespace
{
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    std::string_view
    get_download_folder(AddTorrent::Reader const & _reader)
    {
        auto location = _reader.getDownloadFolder();
        std::string_view result(location.cStr(), location.size());
        return result;
    }

    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    std::filesystem::path
    get_largest_file(
        libtorrent::file_storage const & _files, std::filesystem::path const & _temp_dir)
    {
        int max_index(-1);
        boost::int64_t max_size(-1);
        for (int i(0); i < _files.num_files(); ++i)
        {
            if (_files.file_size(i) > max_size)
            {
                max_index = i;
                max_size = _files.file_size(i);
            }
        }
        std::filesystem::path const result(_files.file_path(max_index, _temp_dir));
        return result;
    }
}
