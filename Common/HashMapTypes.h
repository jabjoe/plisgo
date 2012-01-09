/*
	Copyright 2011 Eurocom Entertainment Software Limited

    This file is part of Plisgo's Utils.

    Eurocom Entertainment Software Limited, hereby disclaims all
    copyright interest in “Plisgo's Utils” written by Joe Burmeister.

    PlisgoFSLib is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

    Plisgo's Utils is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
	License along with Plisgo's Utils.  If not, see
	<http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <boost/unordered_map.hpp>


struct iequal_to : std::binary_function<std::wstring, std::wstring, bool>
{
    bool operator()(std::wstring const& x,
        std::wstring const& y) const
    {
        return boost::algorithm::iequals(x, y, std::locale());
    }
};

struct ihash : std::unary_function<std::wstring, std::size_t>
{
    std::size_t operator()(std::wstring const& x) const
    {
        std::size_t seed = 0;
        std::locale locale;

        for(std::wstring::const_iterator it = x.begin();
            it != x.end(); ++it)
        {
            boost::hash_combine(seed, std::toupper(*it, locale));
        }

        return seed;
    }
};

template<typename T, typename A = std::allocator<std::pair<std::wstring, T> > >
class WStringIMap : public boost::unordered_map<std::wstring, T, ihash, iequal_to, A>	
{
};
