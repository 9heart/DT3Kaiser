#ifndef DT3_KAISERMATH
#define DT3_KAISERMATH
//==============================================================================
///	
///	File: KaiserWorld.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/Types/Base/BaseInclude.hpp"
#include "DT3Core/Types/Math/Matrix4.hpp"
#include "DT3Core/Types/Math/Matrix3.hpp"
#include "DT3Core/Types/Math/MoreMath.hpp"
#include "Kaiser/KaiserCommon.hpp"

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
//==============================================================================

Matrix4 player_card_transform   (Player p, DTint card_num, DTint total_card_num);
Matrix4 trick_card_transform    (Player p, DTint trick_num);
Matrix4 played_card_transform   (Player p);
Matrix4 dealer_card_transform   (Player p);
Matrix4 pass_card_transform     (Player p);

Matrix3 flip_over               (const Matrix3 &t);

//==============================================================================
//==============================================================================

} // DT3

#endif
