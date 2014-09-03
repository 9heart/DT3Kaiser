//==============================================================================
///	
///	File: KaiserInteraction.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserInteraction.hpp"
#include "Kaiser/KaiserObjectCard.hpp"

#include "DT3Core/Types/FileBuffer/ArchiveObjectQueue.hpp"
#include "DT3Core/Types/FileBuffer/ArchiveData.hpp"
#include "DT3Core/Types/GUI/GUITouchEvent.hpp"
#include "DT3Core/Types/Utility/ConsoleStream.hpp"
#include "DT3Core/System/Factory.hpp"
#include "DT3Core/System/System.hpp"
#include "DT3Core/System/Application.hpp"
#include "DT3Core/Devices/DeviceGraphics.hpp"
#include "DT3Core/Objects/GUIObject.hpp"
#include "DT3Core/Objects/CameraObject.hpp"
#include "DT3Core/World/World.hpp"

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
/// Register with object factory
//==============================================================================

IMPLEMENT_FACTORY_CREATION_COMPONENT(KaiserInteraction,"GUIBehaviour","ComponentAdapter")
IMPLEMENT_PLUG_NODE(KaiserInteraction)

//==============================================================================
//==============================================================================

BEGIN_IMPLEMENT_PLUGS(KaiserInteraction)
END_IMPLEMENT_PLUGS

//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserInteraction::KaiserInteraction (void)
    :   _allow_interactions (false)
{

}
		
KaiserInteraction::KaiserInteraction (const KaiserInteraction &rhs)
    :   ComponentBase       (rhs),
        _allow_interactions (rhs._allow_interactions)
{

}

KaiserInteraction & KaiserInteraction::operator = (const KaiserInteraction &rhs)
{
    // Make sure we are not assigning the class to itself
    if (&rhs != this) {        
		ComponentBase::operator = (rhs);
        
        _allow_interactions = rhs._allow_interactions;
    }
    return (*this);
}
			
KaiserInteraction::~KaiserInteraction (void)
{

}

//==============================================================================
//==============================================================================

void KaiserInteraction::initialize (void)
{
	ComponentBase::initialize();
}

//==============================================================================
//==============================================================================

void KaiserInteraction::archive (const std::shared_ptr<Archive> &archive)
{
    ComponentBase::archive(archive);

	archive->push_domain (class_id());
    
    archive->pop_domain ();
}

//==============================================================================
//==============================================================================

void KaiserInteraction::add_to_owner (ObjectBase *owner)
{
    ComponentBase::add_to_owner(owner);
    
    GUIObject *gui = checked_cast<GUIObject*>(owner);
    if (gui) {
        gui->touches_began_callbacks().add(make_callback(this, &type::touches_began));
        gui->touches_moved_callbacks().add(make_callback(this, &type::touches_moved));
        gui->touches_ended_callbacks().add(make_callback(this, &type::touches_ended));
        gui->touches_cancelled_callbacks().add(make_callback(this, &type::touches_cancelled));
    }
}

void KaiserInteraction::remove_from_owner (void)
{
    GUIObject *gui = checked_cast<GUIObject*>(owner());
    if (gui) {
        gui->touches_began_callbacks().remove(make_callback(this, &type::touches_began));
        gui->touches_moved_callbacks().remove(make_callback(this, &type::touches_moved));
        gui->touches_ended_callbacks().remove(make_callback(this, &type::touches_ended));
        gui->touches_cancelled_callbacks().remove(make_callback(this, &type::touches_cancelled));
    }

    ComponentBase::remove_from_owner();
}

//==============================================================================
//==============================================================================

void KaiserInteraction::event_to_ray(GUITouchEvent *event, Vector3 &src, Vector3 &dst)
{
    DTfloat width = System::renderer()->screen_width();
    DTfloat height = System::renderer()->screen_height();
    
    Vector2 pos = event->position();
    
    std::shared_ptr<CameraObject> camera = System::application()->world()->camera();

    // Build a ray used for mouse interactions 
    DTfloat x = static_cast<DTfloat>(pos.x) / width * 2.0F - 1.0F;              // -1.0 to 1.0
    DTfloat y = static_cast<DTfloat>(height - pos.y) / height * 2.0F - 1.0F;    // -1.0 to 1.0
    
    src = camera->unproject_point( Vector3(x,y,-1.0F) );
    dst = camera->unproject_point( Vector3(x,y,1.0F) );
}

DTboolean KaiserInteraction::card_intersection_pos(std::shared_ptr<KaiserObjectCard> card, const Vector3 &src, const Vector3 &dst, Vector3 &pos)
{
    // Convert to local card coordinates
    Vector3 local_src, local_dst;
    local_src = card->transform_inversed() * src;
    local_dst = card->transform_inversed() * dst;

    // Intersect card at z = 0 in local space
    DTfloat t = (0.0F - local_src.z)/(local_dst.z - local_src.z);
    Vector3 local_pos = (local_dst - local_src) * t + local_src;
    
    // Back to world coordinates
    pos = card->transform() * local_pos;
    
    DTfloat width = 256.0F/370.0F, height = 1.0F;

    return (local_pos.x > -width) && (local_pos.x < width) && (local_pos.y > -height) && (local_pos.y < height);
}

//==============================================================================
//==============================================================================

void KaiserInteraction::touches_began (GUITouchEvent *event)
{
    GUIObject *gui = checked_cast<GUIObject*>(owner());
    if (!gui)
        return;

    if (!_allow_interactions || gui->state() == GUIObject::STATE_DISABLED)
        return;
    
    Vector3 src, dst;
    event_to_ray(event, src, dst);
    
    // Reset card position and card
    _card.reset();
    _card_pos.x = 0.0F;
    _card_pos.y = 0.0F;
    _card_pos.z = -std::numeric_limits<DTfloat>::infinity();
    
    
    const std::list<std::shared_ptr<WorldNode>>& nodes = owner()->world()->nodes();
    for (auto &i : nodes) {
        std::shared_ptr<KaiserObjectCard> card = checked_cast<KaiserObjectCard>(i);
        if (card) {
            Vector3 pos;
            
            // Check if you can interact
            if ( (!card->can_interact() || !card->can_use()) && !card->is_trick())
                continue;
            
            // Check if card clicked
            DTboolean selected = card_intersection_pos(card, src, dst, pos);
            if (!selected)
                continue;

            // Closest card
            if (pos.z > _card_pos.z) {
                _card_pos = pos;
                _card = card;

                LOG_MESSAGE << "Selected card: " << _card->value() << " - " << _card->suit();
            }

        }
    }

    // Check if you can interact
    if (_card && _card->is_trick()) {

        if (_trick_cb)
            (*_trick_cb)(_card, true);

    }
    
}

void KaiserInteraction::touches_moved (GUITouchEvent *event)
{
    GUIObject *gui = checked_cast<GUIObject*>(owner());
    if (!gui)
        return;
        
    if (!_allow_interactions || !_card || gui->state() == GUIObject::STATE_DISABLED)
        return;
    
    // Check if you can interact
    if (_card->is_trick()) {

//        if (_trick_cb)
//            (*_trick_cb)(_card, true);

    } else if (_card->can_interact() && _card->can_use()) {
        Vector3 src, dst;
        event_to_ray(event, src, dst);

        // Intersect card at z = 0 in local space
        DTfloat t = (_card_pos.z - src.z)/(dst.z - src.z);
        Vector3 local_pos = (dst - src) * t + src;

        _card->set_translation(_card->translation() + local_pos - _card_pos);
        _card_pos = local_pos;
    }

}

void KaiserInteraction::touches_ended (GUITouchEvent *event)
{
    GUIObject *gui = checked_cast<GUIObject*>(owner());
    if (!gui)
        return;
        
    if (!_allow_interactions || !_card || gui->state() == GUIObject::STATE_DISABLED)
        return;
    
    // Check if you can interact
    if ( (!_card->can_interact() || !_card->can_use()) && !_card->is_trick())
        return;

    if (_card->is_trick()) {

        if (_trick_cb)
            (*_trick_cb)(_card, false);

    } else if (_card->can_interact() && _card->can_use()) {
        Vector3 src, dst;
        event_to_ray(event, src, dst);

        // Intersect card at z = 0 in local space
        DTfloat t = (_card_pos.z - src.z)/(dst.z - src.z);
        Vector3 local_pos = (dst - src) * t + src;

        _card->set_translation(_card->translation() + local_pos - _card_pos);
        _card_pos = local_pos;
        
        // Check if card played
        if (_card_pos.y > -3.5F) {
            (*_cb)(_card);
        } else {
            _card->restore_transform();
        }

        _card.reset();
        _card_pos.x = 0.0F;
        _card_pos.y = 0.0F;
        _card_pos.z = -std::numeric_limits<DTfloat>::infinity();
    }

}

void KaiserInteraction::touches_cancelled (GUITouchEvent *event)
{
    GUIObject *gui = checked_cast<GUIObject*>(owner());
    if (!gui)
        return;

    if (!_allow_interactions || !_card || gui->state() == GUIObject::STATE_DISABLED)
        return;

    // Reset card position and card
    _card->restore_transform();

    _card.reset();
    _card_pos.x = 0.0F;
    _card_pos.y = 0.0F;
    _card_pos.z = -std::numeric_limits<DTfloat>::infinity();

}

//==============================================================================
//==============================================================================

} // DT3

