//==============================================================================
///	
///	File: KaiserObjectCard.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserObjectCard.hpp"
#include "DT3Core/System/Factory.hpp"
#include "DT3Core/Types/FileBuffer/Archive.hpp"
#include "DT3Core/Types/FileBuffer/ArchiveData.hpp"
#include "DT3Core/Objects/CameraObject.hpp"
#include "DT3Core/Resources/ResourceTypes/MaterialResource.hpp"
#include "DT3Core/Resources/ResourceTypes/ShaderResource.hpp"
#include "DT3Core/World/World.hpp"
#include "DT3Core/Types/Animation/PropertyAnimator.hpp"
#include "Kaiser/KaiserWorld.hpp"

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
/// Register with object factory
//==============================================================================

IMPLEMENT_FACTORY_CREATION_PLACEABLE(KaiserObjectCard,"Kaiser","EdCameraObjectAdapter")
IMPLEMENT_PLUG_NODE(KaiserObjectCard)

//==============================================================================
//==============================================================================

BEGIN_IMPLEMENT_PLUGS(KaiserObjectCard)
END_IMPLEMENT_PLUGS

//==============================================================================
//==============================================================================

struct Tile {
    DTubyte row,col;
};

Tile tiles[] = {
    1,  0,  //  Two Club
    1,  1,  //  Two Diamond
    1,  2,  //  Two Heart
    1,  3,  //  Two Spade
    1,  4,  //  Three Club
    1,  5,  //  Three Diamond
    1,  6,  //  Three Heart
    1,  7,  //  Three Spade
    2,  0,  //  Four Club
    2,  1,  //  Four Diamond
    2,  2,  //  Four Heart
    2,  3,  //  Four Spade
    2,  4,  //  Five Club
    2,  5,  //  Five Diamond
    2,  6,  //  Five Heart
    2,  7,  //  Five Spade
    3,  0,  //  Six Club
    3,  1,  //  Six Diamond
    3,  2,  //  Six Heart
    3,  3,  //  Six Spade
    3,  4,  //  Seven Club
    3,  5,  //  Seven Diamond
    3,  6,  //  Seven Heart
    3,  7,  //  Seven Spade
    4,  0,  //  Eight Club
    4,  1,  //  Eight Diamond
    4,  2,  //  Eight Heart
    4,  3,  //  Eight Spade
    4,  4,  //  Nine Club
    4,  5,  //  Nine Diamond
    4,  6,  //  Nine Heart
    4,  7,  //  Nine Spade
    0,  0,  //  Ten Club
    0,  1,  //  Ten Diamond
    0,  2,  //  Ten Heart
    0,  3,  //  Ten Spade
    5,  1,  //  Jack Club
    5,  2,  //  Jack Diamond
    5,  3,  //  Jack Heart
    5,  4,  //  Jack Spade
    6,  1,  //  Queen Club
    6,  2,  //  Queen Diamond
    6,  3,  //  Queen Heart
    6,  4,  //  Queen Spade
    5,  5,  //  King Club
    5,  6,  //  King Diamond
    5,  7,  //  King Heart
    6,  0,  //  King Spade
    0,  4,  //  Ace Club
    0,  5,  //  Ace Diamond
    0,  6,  //  Ace Heart
    0,  7,  //  Ace Spade
    5,  0,  //  Back
};

//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserObjectCard::KaiserObjectCard (void)
    :   _can_interact   (true),
        _can_use        (true),
        _is_trick       (false)
{

}
		
KaiserObjectCard::KaiserObjectCard (const KaiserObjectCard &rhs)
    :   PlaceableObject         (rhs)
{

}

KaiserObjectCard & KaiserObjectCard::operator = (const KaiserObjectCard &rhs)
{
    // Make sure we are not assigning the class to itself
    if (&rhs != this) { 
		PlaceableObject::operator = (rhs);
    }
    return (*this);
}
			
KaiserObjectCard::~KaiserObjectCard (void)
{	

}

//==============================================================================
//==============================================================================

void KaiserObjectCard::initialize (void)
{
	PlaceableObject::initialize();
    
    set_streamable(false);
}

//==============================================================================
//==============================================================================

void KaiserObjectCard::archive (const std::shared_ptr<Archive> &archive)
{
    PlaceableObject::archive(archive);

	archive->push_domain (class_id ());
    archive->pop_domain ();
}

//==============================================================================
//==============================================================================

void KaiserObjectCard::draw (const std::shared_ptr<CameraObject> &camera, DrawBatcher *b, const DTfloat lag)
{
    const DTfloat width = 256.0F/370.0F, height = 1.0F;
    
    Color4b c;
    Matrix4 t = transform();
    
    if ( Vector3::dot(camera->orientation().z_axis(), orientation().z_axis()) > 0.0F ) {
    
        // Front
        if (!_can_use)
            c = Color4b(0.5F,0.5F,0.5F,1.0F);
        else
            c = Color4b(1.0F,1.0F,1.0F,1.0F);

//        b->batch_begin( camera,
//                        _front,
//                        _shader,
//                        transform(),
//                        DT3GL_PRIM_TRI_STRIP,
//                        DrawBatcher::FMT_V | DrawBatcher::FMT_T0 | DrawBatcher::FMT_C,
//                        12);

//        b->add().v(-width,+height,0.0F)  .t0(0.0F,1.0F)  .c(c);
//        b->add().v(-width,-height,0.0F)  .t0(0.0F,0.0F)  .c(c);
//        b->add().v(+width,+height,0.0F)  .t0(1.0F,1.0F)  .c(c);
//        b->add().v(+width,-height,0.0F)  .t0(1.0F,0.0F)  .c(c);

        b->add().v(t * Vector3(-width,-height,0.0F))  .t0(_texcoords.plus_x(),_texcoords.minus_y())    .c(c);
        b->add().v(t * Vector3(+width,+height,0.0F))  .t0(_texcoords.minus_x(),_texcoords.plus_y())      .c(c);
        b->add().v(t * Vector3(-width,+height,0.0F))  .t0(_texcoords.plus_x(),_texcoords.plus_y())     .c(c);
        
        b->add().v(t * Vector3(-width,-height,0.0F))  .t0(_texcoords.plus_x(),_texcoords.minus_y())    .c(c);
        b->add().v(t * Vector3(+width,-height,0.0F))  .t0(_texcoords.minus_x(),_texcoords.minus_y())     .c(c);
        b->add().v(t * Vector3(+width,+height,0.0F))  .t0(_texcoords.minus_x(),_texcoords.plus_y())      .c(c);

        
//        b->batch_end();

    } else {
    
        c = Color4b(1.0F,1.0F,1.0F,1.0F);

//        // Back
//        b->batch_begin( camera,
//                        _back,
//                        _shader,
//                        transform(),
//                        DT3GL_PRIM_TRI_STRIP,
//                        DrawBatcher::FMT_V | DrawBatcher::FMT_T0 | DrawBatcher::FMT_C,
//                        12);

        b->add().v(t * Vector3(+width,-height,0.0F))  .t0(_texcoords_back.plus_x(),_texcoords_back.minus_y())    .c(c);
        b->add().v(t * Vector3(-width,+height,0.0F))  .t0(_texcoords_back.minus_x(),_texcoords_back.plus_y())      .c(c);
        b->add().v(t * Vector3(+width,+height,0.0F))  .t0(_texcoords_back.plus_x(),_texcoords_back.plus_y())     .c(c);
        
        b->add().v(t * Vector3(+width,-height,0.0F))  .t0(_texcoords_back.plus_x(),_texcoords_back.minus_y())    .c(c);
        b->add().v(t * Vector3(-width,-height,0.0F))  .t0(_texcoords_back.minus_x(),_texcoords_back.minus_y())     .c(c);
        b->add().v(t * Vector3(-width,+height,0.0F))  .t0(_texcoords_back.minus_x(),_texcoords_back.plus_y())      .c(c);
        
//        b->batch_end();
    }
    
    b->flush();
}

void KaiserObjectCard::draw_shadow (const std::shared_ptr<CameraObject> &camera, DrawBatcher *b, const DTfloat lag)
{
    const DTfloat width = 256.0F/370.0F, height = 1.0F;
    const Color4b c(1.0F,1.0F,1.0F,1.0F);
    
    // Shadow
    Matrix4 t = transform();
    Vector3 translation = t.translation();
    t.set_translation(Vector3(translation.x + translation.z, translation.y - translation.z, 0.0F));
    
    b->add().v(t * Vector3(-width,-height,0.0F))  .t0(0.0F,0.0F)  .c(c);
    b->add().v(t * Vector3(+width,+height,0.0F))  .t0(1.0F,1.0F)  .c(c);
    b->add().v(t * Vector3(-width,+height,0.0F))  .t0(0.0F,1.0F)  .c(c);
    
    b->add().v(t * Vector3(-width,-height,0.0F))  .t0(0.0F,0.0F)  .c(c);
    b->add().v(t * Vector3(+width,-height,0.0F))  .t0(1.0F,0.0F)  .c(c);
    b->add().v(t * Vector3(+width,+height,0.0F))  .t0(1.0F,1.0F)  .c(c);
    
}

//==============================================================================
//==============================================================================

void KaiserObjectCard::restore_transform (void)
{
    // Shuffle animation first
    auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(shared_from_this()),
                                        &PlaceableObject::transform,
                                        &PlaceableObject::set_transform);

    h->append(_save_transform, 0.2F, 0.0F, std::make_shared<PropertyAnimatorCard>());
}

//==============================================================================
//==============================================================================

void KaiserObjectCard::load (const FilePath &material_path, const std::string &name, Suit suit, Card value)
{
    set_name(name);
    _suit = suit;
    _value = value;

    DTint index = _value * 4 + _suit;

    _texcoords.set( tiles[index].col * 256.0F / 2048.0F,
                    (tiles[index].col+1) * 256.0F / 2048.0F,
                    1.0F - tiles[index].row * 256.0F / 2048.0F,
                    1.0F - (tiles[index].row+1) * 256.0F / 2048.0F);

    _texcoords_back.set( tiles[13*4].col * 256.0F / 2048.0F,
                    (tiles[13*4].col+1) * 256.0F / 2048.0F,
                    1.0F - tiles[13*4].row * 256.0F / 2048.0F,
                    1.0F - (tiles[13*4].row+1) * 256.0F / 2048.0F);
}

//==============================================================================
//==============================================================================

void KaiserObjectCard::add_to_world(World *world)
{
    PlaceableObject::add_to_world(world);
    
    KaiserWorld *w = checked_cast<KaiserWorld*>(world);
    
    w->register_for_draw_batched(this, make_callback(this, &type::draw));
    w->register_for_shadow_draw_batched(this, make_callback(this, &type::draw_shadow));
}

void KaiserObjectCard::remove_from_world (void)
{
    KaiserWorld *w = checked_cast<KaiserWorld*>(world());

    w->unregister_for_draw_batched(this, make_callback(this, &type::draw));
    w->unregister_for_shadow_draw_batched(this, make_callback(this, &type::draw_shadow));

    PlaceableObject::remove_from_world();
}

//==============================================================================
//==============================================================================

} // DT3

