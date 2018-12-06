#ifndef _MSC_VER
#   ident "$Id: $"
#endif

/*
    Created: 05 December 2018

    Author: Tony Thompson <ajthomps51@gmail.com>
*/

#include "vpn.h"
#include <iostream>
#include <boost/process.hpp>

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
        : stopped_(true)
    {

    }
    
    //**********************************************************************************************
    // Notes:
    //----------------------------------------------------------------------------------------------
    void
    vpn::start()
    {
        thread_ = boost::thread([this]() { this->thread_proc(); });
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
                start = std::chrono::system_clock::now();
                boost::process::child child("openvpn --config /etc/openvpn/nyc.ovpn",
                    boost::process::std_out > "piavpn.out", boost::process::std_err > "piavpn.err");
                while (child.running())
                {
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
                }
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
