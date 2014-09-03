//==============================================================================
///	
///	File: KaiserMath.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserMath.hpp"
#include "DT3Core/System/System.hpp"
#include "DT3Core/Devices/DeviceGraphics.hpp"


//==============================================================================
//==============================================================================

#define SHOW_CARDS 0

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
//==============================================================================

Matrix4 player_card_transform (Player p, DTint card_num, DTint total_card_num)
{
    DTfloat DISTANCE = 5.7F;
    DTfloat SPACING_H = 1.0F;
    DTfloat SPACING_H2 = 0.5F;
    DTfloat SPACING_V = 0.5F;
    
    DTfloat screen_aspect = System::renderer()->screen_aspect();
    DTfloat squeeze = MoreMath::change_range_clamp_min(screen_aspect, 640.0F/960.0F, 640.0F/1136.0F, 1.0F, 0.9F);   // iPhone 4 to iPhone 5
    
    switch (p) {
        case PLAYER_0: {
                DTfloat half_width = (total_card_num - 1) * SPACING_H * 0.5F * squeeze;
                Vector3 pos0 = Vector3(-half_width, -DISTANCE, 0.2F);
                Vector3 pos1 = Vector3(half_width, -DISTANCE, 0.2F);
                
                DTfloat t = (DTfloat) card_num / (DTfloat) (total_card_num-1);
                Vector3 translation = pos0 + (pos1 - pos0) * t;
                Matrix3 orientation = Matrix3::set_rotation_z(0.0F + (t-0.5F) * 0.2F);
            
                translation.z += t * 0.0001F;
            
                return Matrix4(orientation,translation);
            
            } break;
            
        case PLAYER_1: {
                DTfloat half_height = (total_card_num - 1) * SPACING_V * 0.5F;
                Vector3 pos0 = Vector3(-DISTANCE*screen_aspect, half_height, 0.2F);
                Vector3 pos1 = Vector3(-DISTANCE*screen_aspect, -half_height, 0.2F);
                
                DTfloat t = (DTfloat) card_num / (DTfloat) (total_card_num-1);
                Vector3 translation = pos0 + (pos1 - pos0) * t;
#if SHOW_CARDS
                Matrix3 orientation = Matrix3::set_rotation_z(PI * 0.5F - (t-0.5F) * 0.2F);
#else
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(PI * 0.5F - (t-0.5F) * 0.2F));
#endif

                translation.z += t * 0.0001F;

                return Matrix4(orientation,translation);

            } break;
            
        case PLAYER_2: {
                DTfloat half_width = (total_card_num - 1) * SPACING_H2 * 0.5F;
                Vector3 pos0 = Vector3(half_width, DISTANCE, 0.2F);
                Vector3 pos1 = Vector3(-half_width, DISTANCE, 0.2F);
                
                DTfloat t = (DTfloat) card_num / (DTfloat) (total_card_num-1);
                Vector3 translation = pos0 + (pos1 - pos0) * t;
#if SHOW_CARDS
                Matrix3 orientation = Matrix3::set_rotation_z(PI - (t-0.5F) * 0.2F);
#else
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(PI - (t-0.5F) * 0.2F));
#endif
            
                translation.z += t * 0.0001F;

                return Matrix4(orientation,translation);

            } break;
            
        case PLAYER_3: {
                DTfloat half_height = (total_card_num - 1) * SPACING_V * 0.5F;
                Vector3 pos0 = Vector3(DISTANCE*screen_aspect, -half_height, 0.2F);
                Vector3 pos1 = Vector3(DISTANCE*screen_aspect, half_height, 0.2F);
                
                DTfloat t = (DTfloat) card_num / (DTfloat) (total_card_num-1);
                Vector3 translation = pos0 + (pos1 - pos0) * t;
#if SHOW_CARDS
                Matrix3 orientation = Matrix3::set_rotation_z(PI * 0.5F * 3.0F - (t-0.5F) * 0.2F);
#else
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(PI * 0.5F * 3.0F - (t-0.5F) * 0.2F));
#endif

                translation.z += t * 0.0001F;

                return Matrix4(orientation,translation);

            } break;
    }

    return Matrix4::identity();
}

//==============================================================================
//==============================================================================

Matrix4 trick_card_transform (Player p, DTint trick_num)
{
    DTfloat DISTANCE = 3.5F;
    DTfloat SPACING_H = 0.3F;
    DTfloat SPACING_V = 0.3F;
    
    DTfloat screen_aspect = System::renderer()->screen_aspect();
    
    switch (p) {
        case PLAYER_0: {
                DTfloat half_width = 8 * SPACING_H * 0.5F;
                Vector3 translation = Vector3(-half_width * screen_aspect, -DISTANCE, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.1F));
            
                translation.x += trick_num * SPACING_H;
            
                return Matrix4(orientation,translation);
            
            } break;
            
        case PLAYER_1: {
                DTfloat half_height = 8 * SPACING_V * 0.5F;
                Vector3 translation = Vector3(-DISTANCE * screen_aspect, half_height, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.1F + PI*0.5F));
            
                translation.y -= trick_num * SPACING_V;
            
                return Matrix4(orientation,translation);

            } break;
            
        case PLAYER_2: {
                DTfloat half_width = 8 * SPACING_H * 0.5F;
                Vector3 translation = Vector3(half_width * screen_aspect, DISTANCE, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.1F));
            
                translation.x += trick_num * SPACING_H;
            
                return Matrix4(orientation,translation);

            } break;
            
        case PLAYER_3: {
                DTfloat half_height = 8 * SPACING_V * 0.5F;
                Vector3 translation = Vector3(DISTANCE * screen_aspect, -half_height, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.1F + PI*0.5F));
            
                translation.y += trick_num * SPACING_V;
            
                return Matrix4(orientation,translation);

            } break;
    }

    return Matrix4::identity();
}

//==============================================================================
//==============================================================================

Matrix4 played_card_transform (Player p)
{
    DTfloat DISTANCE = 1.0F;
    
    switch (p) {
        case PLAYER_0: {
                Vector3 translation = Vector3(0.0F, -DISTANCE, 0.01F);
                Matrix3 orientation = Matrix3::set_rotation_z(0.5F * (MoreMath::random_MT_float() - 0.5F) );
            
                return Matrix4(orientation,translation);
            } break;
            
        case PLAYER_1: {
                Vector3 translation = Vector3(-DISTANCE, 0.0F, 0.01F);
                Matrix3 orientation = Matrix3::set_rotation_z(0.5F * (MoreMath::random_MT_float() - 0.5F) );
            
                return Matrix4(orientation,translation);
            } break;
            
        case PLAYER_2: {
                Vector3 translation = Vector3(0.0F, DISTANCE, 0.01F);
                Matrix3 orientation = Matrix3::set_rotation_z(0.5F * (MoreMath::random_MT_float() - 0.5F) );
            
                return Matrix4(orientation,translation);
            } break;
            
        case PLAYER_3: {
                Vector3 translation = Vector3(DISTANCE, 0.0F, 0.01F);
                Matrix3 orientation = Matrix3::set_rotation_z(0.5F * (MoreMath::random_MT_float() - 0.5F) );
            
                return Matrix4(orientation,translation);
            } break;
    }

    return Matrix4::identity();
}

//==============================================================================
//==============================================================================

Matrix4 dealer_card_transform (Player p)
{
    DTfloat DISTANCE = 3.0F;

    DTfloat screen_aspect = System::renderer()->screen_aspect();

    switch (p) {
        case PLAYER_0: {
                Vector3 translation = Vector3(0.0F, -DISTANCE, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.1F));
            
                return Matrix4(orientation,translation);
            } break;
            
        case PLAYER_1: {
                Vector3 translation = Vector3(-DISTANCE * screen_aspect, 0.0F, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(PI * 0.5F + 0.1F));
            
                return Matrix4(orientation,translation);
            } break;
            
        case PLAYER_2: {
                Vector3 translation = Vector3(0.0F, DISTANCE, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.1F));
            
                return Matrix4(orientation,translation);
            } break;
            
        case PLAYER_3: {
                Vector3 translation = Vector3(DISTANCE * screen_aspect, 0.0F, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(PI * 0.5F * 3.0F + 0.1F));
            
                return Matrix4(orientation,translation);
            } break;
    }
}

//==============================================================================
//==============================================================================

Matrix4 pass_card_transform (Player p)
{
    DTfloat DISTANCE = 3.0F;

    switch (p) {
        case PLAYER_0: {
                Vector3 translation = Vector3(0.0F, DISTANCE, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.5F * (MoreMath::random_MT_float() - 0.5F) ));

                return Matrix4(orientation,translation);
            } break;

        case PLAYER_1: {
                Vector3 translation = Vector3(DISTANCE, 0.0F, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.5F * (MoreMath::random_MT_float() - 0.5F) ));

                return Matrix4(orientation,translation);
            } break;

        case PLAYER_2: {
                Vector3 translation = Vector3(0.0F, -DISTANCE, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.5F * (MoreMath::random_MT_float() - 0.5F) ));

                return Matrix4(orientation,translation);
            } break;

        case PLAYER_3: {
                Vector3 translation = Vector3(-DISTANCE, 0.0F, 0.0F);
                Matrix3 orientation = flip_over(Matrix3::set_rotation_z(0.5F * (MoreMath::random_MT_float() - 0.5F) ));

                return Matrix4(orientation,translation);
            } break;

    }
}

//==============================================================================
//==============================================================================

Matrix3 flip_over (const Matrix3 &t)
{
    return Matrix3::set_rotation_x(PI) * t;
}

//==============================================================================
//==============================================================================

} // DT3

