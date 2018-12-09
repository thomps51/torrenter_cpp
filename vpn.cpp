#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 05 December 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

#include "vpn.h"
#include <iostream>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <tsl/utility/get_data_from_url.h>

namespace
{
    std::string
    get_external_ip();

    void
    validate_ip_address(std::string_view _ip_address);
}

namespace torrenter
{
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    vpn::~vpn()
    {
        stop();
    }

    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    vpn::vpn(std::filesystem::path const & _config_file)
        : running_(false)
        , stopped_(true)
    {

    }
    
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    vpn::start()
    {
        using tsl::utility::get_data_from_url;
        auto const starting_ip_address{get_external_ip()};
        //validate_ip_address(starting_ip_address);
        thread_ = boost::thread([this]() { this->thread_proc(); });
        while (!running_.load())
        {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
        }
    }
    
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    vpn::stop()
    {
        if (stopped_.load())
        {
            return;
        }
        thread_.interrupt();
        thread_.join();
    }
    
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    vpn::thread_proc()
    {
        stopped_ = false;
        try
        {
            auto start = std::chrono::system_clock::now();
            bool first(true);
            for (;;)
            {
                if (!first && std::chrono::system_clock::now() - start < std::chrono::seconds(5))
                {
                    std::cerr << "openvpn failed within 5 seconds of previous failure: check logs"
                              << std::endl;
                    std::exit(EXIT_FAILURE);
                }
                first = false;
                auto const starting_ip_address{get_external_ip()};
                //validate_ip_address(starting_ip_address);
                start = std::chrono::system_clock::now();
                boost::process::child child("openvpn --config /etc/openvpn/nyc.ovpn",
                    boost::process::std_out > "piavpn.out", boost::process::std_err > "piavpn.err");
                int count{0};
                for (;;)
                {
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
                    auto ip_address{get_external_ip()};
                    //validate_ip_address(ip_address);
                    if (ip_address != starting_ip_address)
                    {
                        ip_address_ = std::move(ip_address);
                        break;
                    }
                    if (++count > 10)
                    {
                        throw std::runtime_error(
                            "Cannot detect change in ip address when connecting to vpn");
                    }
                }
                while (child.running())
                {
                    running_ = true;
                    if (get_external_ip() != ip_address_)
                    {
                        std::cout
                            << "External IP address has changed while vpn is running. Exitting"
                            << std::endl;
                        child.terminate();
                        std::exit(EXIT_FAILURE);
                    }
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));
                }
                running_ = false;
                std::cout << "VPN process has exited, retrying after 1 second" << std::endl;
                boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
            }
        }
        catch (boost::thread_interrupted &)
        {
            std::cout << "VPN thread inturrupted, exiting" << std::endl;
        }
        catch (std::exception & _except)
        {
            std::cout << "Got other exception: " << _except.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }
        catch (...)
        {
            std::cout << "Unknown exception" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        stopped_ = true;
    }
}

namespace
{
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    std::string
    get_external_ip()
    {
        return tsl::utility::get_data_from_url("http://myexternalip.com/raw").as_string();
    }

    //**********************************************************************************************
    // Notes: This was giving errors as written when passing valid dot-separated IP address - need
    // to look into why this was happening.
    //----------------------------------------------------------------------------------------------
    void
    validate_ip_address(std::string_view _ip_address)
    {
        namespace ip = boost::asio::ip;
        boost::system::error_code error;
        //ip::address::make_address(_ip_address, error);
        ip::address::from_string(std::string(_ip_address));
        //if (error)
        //{
        //    std::string const message("invalid ip address");
        //    throw std::runtime_error(message);
        //}
    }
}
