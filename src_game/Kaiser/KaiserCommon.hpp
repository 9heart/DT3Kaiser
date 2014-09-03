#ifndef DT3_KAISERCOMMON
#define DT3_KAISERCOMMON
//==============================================================================
///	
///	File: KaiserCommon.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/Types/Base/BaseClass.hpp"
#include "DT3Core/Types/Animation/PropertyAnimator.hpp"

namespace DT3 {

//==============================================================================
//==============================================================================

enum Player {
    PLAYER_0 = 0,
    PLAYER_1,
    PLAYER_2,
    PLAYER_3
};

enum Suit {
    SUIT_CLUB = 0,
    SUIT_DIAMOND,
    SUIT_HEART,
    SUIT_SPADE,
    SUIT_NO,
    SUIT_UNDEFINED
};

enum GameState {
    STATE_DEALING,
    STATE_BIDDING,
    STATE_CHECK_BIDDING,
    STATE_DECLARE_TRUMP,
    STATE_DECLARE_FORCE_TRUMP,
    STATE_CHECK_TRUMP,
    STATE_PLAYING,
    STATE_TALLY,
    STATE_IDLE
};

enum Card {
    CARD_2 = 0,
    CARD_3 = 1,
    CARD_4 = 2,
    CARD_5 = 3,
    CARD_6 = 4,
    CARD_7 = 5,
    CARD_8 = 6,
    CARD_9 = 7,
    CARD_10 = 8,
    CARD_JACK = 9,
    CARD_QUEEN = 10,
    CARD_KING = 11,
    CARD_ACE = 12
};

//==============================================================================
//==============================================================================

class PropertyAnimatorCard: public PropertyAnimatorInterpolatorBase<Matrix4> {
    public:
        virtual Matrix4 interp (const Matrix4 &src,const Matrix4 &dst, DTfloat t) {
            
            DTfloat t1 = t;
            DTfloat t2 = t1 * t1;
            DTfloat t3 = t1 * t2;
        
            // interpolate via Hermite spline
            // See Realtime Rendering, 2nd Ed., Page 492
            Vector3 p1,p2,p3,p4;
            p1 = src.translation() * (2.0F * t3 - 3.0F * t2 + 1.0F);
            p2 = Vector3(0.0F,0.0F,3.0F) * (t3 - 2.0F * t2 + t1);

            p3 = Vector3(0.0F,0.0F,-3.0F) * (t3 - t2);
            p4 = dst.translation() * (-2.0F * t3 + 3.0F * t2);
            
            return Matrix4( Matrix3(Quaternion::slerp(Quaternion(src.orientation()), Quaternion(dst.orientation()), t)),
                            p1 + p2 + p3 + p4);
        }
};

class PropertyAnimatorCardUp: public PropertyAnimatorInterpolatorBase<Matrix4> {
    public:
        virtual Matrix4 interp (const Matrix4 &src,const Matrix4 &dst, DTfloat t) {
            
            DTfloat t1 = t;
            DTfloat t2 = t1 * t1;
            DTfloat t3 = t1 * t2;
        
            // interpolate via Hermite spline
            // See Realtime Rendering, 2nd Ed., Page 492
            Vector3 p1,p2,p3,p4;
            p1 = src.translation() * (2.0F * t3 - 3.0F * t2 + 1.0F);
            p2 = Vector3(0.0F,3.0F,0.0F) * (t3 - 2.0F * t2 + t1);

            p3 = Vector3(0.0F,-3.0F,-0.0F) * (t3 - t2);
            p4 = dst.translation() * (-2.0F * t3 + 3.0F * t2);
            
            return Matrix4( Matrix3(Quaternion::slerp(Quaternion(src.orientation()), Quaternion(dst.orientation()), t)),
                            p1 + p2 + p3 + p4);
        }
};

//==============================================================================
//==============================================================================

} // DT3

#endif
