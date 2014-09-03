//==============================================================================
///	
///	File: KaiserBid.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserBid.hpp"

//==============================================================================
//==============================================================================

namespace DT3 {
    
//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserBid::KaiserBid (void)
{
    _bid = 0;
    _no = false;
    _pass = true;
    _trump = SUIT_UNDEFINED;
}

KaiserBid::KaiserBid (DTshort bid, DTboolean no)
{
    _bid = bid;
    _no = no;
    _pass = false;
    _trump = SUIT_UNDEFINED;
}
	
KaiserBid::KaiserBid (const KaiserBid &rhs)
    :   BaseClass(rhs),
        _bid    (rhs._bid),
        _no     (rhs._no),
        _pass   (rhs._pass),
        _trump  (rhs._trump)
{
    
}

KaiserBid& KaiserBid::operator = (const KaiserBid &rhs)
{
    // Make sure we are not assigning the class to itself
    if (&rhs != this) {
		BaseClass::operator = (rhs);

        _bid = rhs._bid;
        _no = rhs._no;
        _pass = rhs._pass;
        _trump = rhs._trump;
    }
    return (*this);
}
		
KaiserBid::~KaiserBid (void)
{

}

//==============================================================================
//==============================================================================

} // DT3

