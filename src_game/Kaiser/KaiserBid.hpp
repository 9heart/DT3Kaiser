#ifndef DT3_KAISERBID
#define DT3_KAISERBID
//==============================================================================
///	
///	File: KaiserBid.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/Types/Base/BaseClass.hpp"
#include "Kaiser/KaiserCommon.hpp"

namespace DT3 {

//==============================================================================
/// Forward declarations
//==============================================================================

//==============================================================================
//==============================================================================

class KaiserBid: public BaseClass {    
    public:
        DEFINE_TYPE(KaiserBid,BaseClass)
		DEFINE_CREATE_AND_CLONE

                                KaiserBid               (void);                         // Pass
                                KaiserBid               (DTshort bid, DTboolean no);    // Bid
    
                                KaiserBid               (const KaiserBid &rhs);
        KaiserBid &             operator =				(const KaiserBid &rhs);
                                ~KaiserBid              (void);
  
    public:
    
        /// Bid amount
        void                    set_bid                 (DTshort bid)       {   _bid = bid;    }

        /// Bid amount
        DTshort                 bid                     (void) const        {   return _bid;    }

        /// Pass flag
        DTboolean               is_pass                 (void) const        {   return _pass;   }

        /// Sets no
        void                    set_no                  (DTboolean no)      {   _no = no;       }

        /// No Bid flag
        DTboolean               is_no                   (void) const        {   return _no;     }


        /// Sets a trump
        void                    set_trump               (Suit trump)        {   _trump = trump; }

        /// Pass flag
        Suit                    trump                   (void) const        {   return _trump;  }

    
    private:
        friend DTboolean		operator ==             (const KaiserBid &a, const KaiserBid &b);
        friend DTboolean		operator !=             (const KaiserBid &a, const KaiserBid &b);
        friend DTboolean		operator <              (const KaiserBid &a, const KaiserBid &b);
        friend DTboolean		operator <=             (const KaiserBid &a, const KaiserBid &b);
        friend DTboolean		operator >              (const KaiserBid &a, const KaiserBid &b);
        friend DTboolean		operator >=             (const KaiserBid &a, const KaiserBid &b);
    
        DTshort     _bid;
        DTboolean   _no;
        DTboolean   _pass;
        Suit        _trump;
};

//==============================================================================
//==============================================================================

inline DTboolean operator == (const KaiserBid &a, const KaiserBid &b) {
    return  (a._bid == b._bid) &&
            (a._no == b._no) &&
            (a._pass == b._pass);
}

inline DTboolean operator != (const KaiserBid &a, const KaiserBid &b) {
    return  (a._bid != b._bid) ||
            (a._no != b._no) ||
            (a._pass != b._pass);
}

inline DTboolean operator < (const KaiserBid &a, const KaiserBid &b) {

    DTuint ba = ((a._bid << 1) | (0x01 & a._no)) * (a._pass ? 0 : 1);
    DTuint bb = ((b._bid << 1) | (0x01 & b._no)) * (b._pass ? 0 : 1);
    
    return ba < bb;
}

inline DTboolean operator <= (const KaiserBid &a, const KaiserBid &b) {

    DTuint ba = ((a._bid << 1) | (0x01 & a._no)) * (a._pass ? 0 : 1);
    DTuint bb = ((b._bid << 1) | (0x01 & b._no)) * (b._pass ? 0 : 1);
    
    return ba <= bb;
}

inline DTboolean operator > (const KaiserBid &a, const KaiserBid &b) {

    DTuint ba = ((a._bid << 1) | (0x01 & a._no)) * (a._pass ? 0 : 1);
    DTuint bb = ((b._bid << 1) | (0x01 & b._no)) * (b._pass ? 0 : 1);
    
    return ba > bb;
}

inline DTboolean operator >= (const KaiserBid &a, const KaiserBid &b) {

    DTuint ba = ((a._bid << 1) | (0x01 & a._no)) * (a._pass ? 0 : 1);
    DTuint bb = ((b._bid << 1) | (0x01 & b._no)) * (b._pass ? 0 : 1);
    
    return ba >= bb;
}

//==============================================================================
//==============================================================================

} // DT3

#endif
