/*
*  This file is part of aasdk library project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <boost/asio.hpp>
#include <mutex>

namespace f1x
{
namespace aasdk
{
namespace io
{

class IOContextWrapper
{
public:
    IOContextWrapper();
    explicit IOContextWrapper(boost::asio::io_context& ioContext);
    explicit IOContextWrapper(boost::asio::io_context::strand& strand);

    template<typename CompletionHandlerType>
    void post(CompletionHandlerType&& handler)
    {
        if(ioContext_ != nullptr)
        {
            boost::asio::post(*ioContext_, std::forward<CompletionHandlerType>(handler));
        }
        else if(strand_ != nullptr)
        {
            boost::asio::post(*strand_, std::forward<CompletionHandlerType>(handler));
        }
    }

    template<typename CompletionHandlerType>
    void dispatch(CompletionHandlerType&& handler)
    {
        if(ioContext_ != nullptr)
        {
            boost::asio::dispatch(*ioContext_, std::forward<CompletionHandlerType>(handler));
        }
        else if(strand_ != nullptr)
        {
            boost::asio::dispatch(*strand_, std::forward<CompletionHandlerType>(handler));
        }
    }

    void reset();
    bool isActive() const;

private:
    boost::asio::io_context* ioContext_;
    boost::asio::io_context::strand* strand_;
};

}
}
}
