//==============================================================================
///	
///	File: KaiserGameController.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserGameController.hpp"
#include "DT3Core/System/Factory.hpp"
#include "DT3Core/System/System.hpp"
#include "DT3Core/System/Application.hpp"
#include "DT3Core/System/Globals.hpp"
#include "DT3Core/Types/FileBuffer/Archive.hpp"
#include "DT3Core/Types/FileBuffer/ArchiveData.hpp"
#include "DT3Core/Types/FileBuffer/FilePath.hpp"
#include "DT3Core/Types/Math/MoreMath.hpp"
#include "DT3Core/Types/GUI/GUILayoutPolicy.hpp"
#include "DT3Core/Types/GUI/GUIGridLayout.hpp"
#include "DT3Core/Types/GUI/GUIAnimKey.hpp"
#include "DT3Core/Types/Utility/ConsoleStream.hpp"
#include "DT3Core/Types/Utility/LatentCall.hpp"
#include "DT3Core/Types/Utility/MoreStrings.hpp"
#include "DT3Core/Types/Utility/Analytics.hpp"
#include "DT3Core/Types/Threads/ThreadMainTaskQueue.hpp"
#include "DT3Core/World/World.hpp"
#include "DT3Core/Objects/GUIController.hpp"
#include "DT3Core/Objects/GUIObject.hpp"
#include "DT3Core/Devices/DeviceAudio.hpp"
#include "DT3Core/Resources/ResourceTypes/FontResource.hpp"
#include "DT3Core/Components/ComponentGUIButton.hpp"
#include "DT3Core/Components/ComponentGUIToggleButton.hpp"
#include "DT3Core/Components/ComponentGUIDrawButton.hpp"
#include "DT3Core/Components/ComponentGUIDrawText.hpp"
#include "DT3Core/Components/ComponentGUIDrawIcon.hpp"
#include "Kaiser/KaiserObjectCard.hpp"
#include "Kaiser/KaiserMath.hpp"
#include "Kaiser/KaiserInteraction.hpp"
#include <algorithm>

#if DTP_USE_PORTAL
    #include "DTPortalSDK/DTPortalLib/DTPortalSDK.hpp"
#endif

#ifndef DT3_EDITOR
    extern void show_twitter    (const std::string &msg);
    extern void show_facebook   (const std::string &msg);
#else
    inline void show_twitter    (const std::string &msg)    {}
    inline void show_facebook   (const std::string &msg)    {}
#endif

//==============================================================================
//==============================================================================

#define FORCE_DEAL 0

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
/// Register with object factory
//==============================================================================

IMPLEMENT_FACTORY_CREATION_PLACEABLE(KaiserGameController,"Kaiser","")
    
//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserGameController::KaiserGameController (void)
    :   _co                 (this, &KaiserGameController::run_co, 1024*1024),
        _us_score           (0),
        _them_score         (0),
        _us_score_round     (0),
        _them_score_round   (0),
        _minimum_bid        (7),
        _human_player       (true),
        _submitted          (false),
        _dismissed_nag      (false),
        _options_set            (false),
        _option_no_trump_bidout (false),
        _option_pass_cards      (false),
        _option_game_over_52    (false),
        _pause_sound_on     (true)
//        _option_steal_5_is_win  (false)
{

}
	
KaiserGameController::KaiserGameController (const KaiserGameController &rhs)
    :   GameController(rhs),
        _co                 (this, &KaiserGameController::run_co, 1024*1024),
        _us_score           (0),
        _them_score         (0),
        _us_score_round     (0),
        _them_score_round   (0),
        _minimum_bid        (7),
        _human_player       (true),
        _submitted          (false),
        _dismissed_nag      (false),
        _pause_sound_on     (true)
{

}

KaiserGameController & KaiserGameController::operator = (const KaiserGameController &rhs)
{
    // Make sure we are not assigning the class to itself
    if (&rhs != this) {
		GameController::operator = (rhs);
    }
    return (*this);
}
		
KaiserGameController::~KaiserGameController (void)
{

}

//==============================================================================
//==============================================================================

void KaiserGameController::initialize (void)
{
	GameController::initialize();

    _minimum_bid = MoreStrings::cast_from_string<DTshort>(Globals::global("APP_MIN_BID"));
    _option_no_trump_bidout = MoreStrings::cast_from_string<DTboolean>(Globals::global("APP_NO_TRUMP_BIDOUT"));
    _option_pass_cards = MoreStrings::cast_from_string<DTboolean>(Globals::global("APP_PASS_CARDS"));
    _option_game_over_52 = MoreStrings::cast_from_string<DTboolean>(Globals::global("APP_GAME_OVER_52"));
//    _option_steal_5_is_win = MoreStrings::cast_from_string<DTboolean>(Globals::global("APP_STEAL_5_TO_WIN"));

    _pause_sound_on = MoreStrings::cast_from_string<DTboolean>(Globals::global("APP_SOUND_ON"));
}

//==============================================================================
//==============================================================================

void KaiserGameController::archive (const std::shared_ptr<Archive> &archive)
{
    GameController::archive(archive);

    archive->push_domain (class_id ());
    
    archive->pop_domain ();
}

void KaiserGameController::archive_done (const std::shared_ptr<Archive> &archive)
{		
	GameController::archive_done (archive);
    
    if (archive->is_writing())
        return;
}

//==============================================================================
//                _                 _   _
//    /\         (_)               | | (_)                
//   /  \   _ __  _ _ __ ___   __ _| |_ _  ___  _ __  ___ 
//  / /\ \ | '_ \| | '_ ` _ \ / _` | __| |/ _ \| '_ \/ __|
// / ____ \| | | | | | | | | | (_| | |_| | (_) | | | \__ \
///_/    \_\_| |_|_|_| |_| |_|\__,_|\__|_|\___/|_| |_|___/
//
//==============================================================================

void KaiserGameController::animate_bid (Player p, const std::shared_ptr<KaiserBid> &bid)
{
    // Bid to message
    std::string msg;
    if (bid->bid() == 12 && bid->is_no())
        msg = "{FMT_MESSAGE}Kaiser!";
    else if (!bid || bid->is_pass())
        msg = "{FMT_MESSAGE}Pass";
    else if (bid->is_no())
        msg = "{FMT_MESSAGE}" + MoreStrings::cast_to_string(bid->bid()) + " No";
    else
        msg = "{FMT_MESSAGE}" + MoreStrings::cast_to_string(bid->bid());
    
    animate_player_message (p, msg, 0.0F);
}

void KaiserGameController::animate_trump (Player p, const std::shared_ptr<KaiserBid> &bid)
{
    // Bid to message
    std::string msg;
    if (!bid || bid->is_no())
        msg = "{FMT_MESSAGE}No";
    else {
    
        switch (bid->trump()) {
            case SUIT_CLUB:     msg = "{FMT_MESSAGE}{CLUB}";    break;
            case SUIT_DIAMOND:  msg = "{FMT_MESSAGE}{DIAMOND}"; break;
            case SUIT_HEART:    msg = "{FMT_MESSAGE}{HEART}";   break;
            case SUIT_SPADE:    msg = "{FMT_MESSAGE}{SPADE}";   break;
            default:
                ASSERT(0);
        }
    
    }
    
    animate_player_message (p, msg);
}

void KaiserGameController::animate_player_turn (Player p)
{
    _turn_player_0_widget->cancel_repeating_anims();
    _turn_player_1_widget->cancel_repeating_anims();
    _turn_player_2_widget->cancel_repeating_anims();
    _turn_player_3_widget->cancel_repeating_anims();

    switch (p) {
        case PLAYER_0:
            _turn_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.3F,1.3F,1.3F))
                .set_repeat();
            
            _turn_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.0F,1.0F,1.0F))
                .set_repeat();
            
            _turn_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _turn_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _turn_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            break;
        
        case PLAYER_1:
            _turn_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _turn_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.3F,1.3F,1.3F))
                .set_repeat();
            
            _turn_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.0F,1.0F,1.0F))
                .set_repeat();

            _turn_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _turn_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            break;

        case PLAYER_2:
            _turn_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));

            _turn_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _turn_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.3F,1.3F,1.3F))
                .set_repeat();
            
            _turn_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.0F,1.0F,1.0F))
                .set_repeat();

            _turn_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            break;

        case PLAYER_3:
            _turn_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));

            _turn_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _turn_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _turn_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.3F,1.3F,1.3F))
                .set_repeat();
            
            _turn_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.0F,1.0F,1.0F))
                .set_repeat();
            break;
    }
}

void KaiserGameController::animate_dealer_turn (Player p)
{
    _dealer_player_0_widget->cancel_repeating_anims();
    _dealer_player_1_widget->cancel_repeating_anims();
    _dealer_player_2_widget->cancel_repeating_anims();
    _dealer_player_3_widget->cancel_repeating_anims();

    switch (p) {
        case PLAYER_0:
            _dealer_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.3F,1.3F,1.3F));
            
            _dealer_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.0F,1.0F,1.0F));
            
            _dealer_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _dealer_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _dealer_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            break;
        
        case PLAYER_1:
            _dealer_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _dealer_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.3F,1.3F,1.3F));
            
            _dealer_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.0F,1.0F,1.0F));

            _dealer_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _dealer_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            break;

        case PLAYER_2:
            _dealer_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));

            _dealer_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _dealer_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.3F,1.3F,1.3F));
            
            _dealer_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.0F,1.0F,1.0F));

            _dealer_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            break;

        case PLAYER_3:
            _dealer_player_0_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));

            _dealer_player_1_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _dealer_player_2_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,0.0F))
                .set_scale(Vector3(0.0F,0.0F,0.0F));
            
            _dealer_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.3F,1.3F,1.3F));
            
            _dealer_player_3_widget->add_anim_key()
                .set_duration(0.3F)
                .set_color(Color4f(1.0F,1.0F,1.0F,1.0F))
                .set_scale(Vector3(1.0F,1.0F,1.0F));
            break;
    }
}


void KaiserGameController::animate_player_message (Player p, const std::string &msg, DTfloat delay)
{
    std::shared_ptr<GUIObject> w;
    
    switch (p) {
        case PLAYER_0:  w = _message_player_0_widget;   break;
        case PLAYER_1:  w = _message_player_1_widget;   break;
        case PLAYER_2:  w = _message_player_2_widget;   break;
        case PLAYER_3:  w = _message_player_3_widget;   break;
    }
    
    w->add_anim_key()
        .set_label(msg)
        .set_duration(0.3F)
        .set_scale(Vector3(1.1F,1.1F,1.1F));

    w->add_anim_key()
        .set_call(make_latent_call(this, &KaiserGameController::play_sound_bubble))
        .set_duration(0.3F)
        .set_scale(Vector3(1.0F,1.0F,1.0F));

    if (delay > 0.0F) {
        w->add_anim_key()
            .set_delay(delay)
            .set_duration(0.3F)
            .set_scale(Vector3(1.1F,1.1F,1.1F));

        w->add_anim_key()
            .set_duration(0.3F)
            .set_scale(Vector3(0.0F,0.0F,0.0F));
    }

}

void KaiserGameController::animate_hide_player_message(Player p)
{
    std::shared_ptr<GUIObject> w;
    
    switch (p) {
        case PLAYER_0:  w = _message_player_0_widget;   break;
        case PLAYER_1:  w = _message_player_1_widget;   break;
        case PLAYER_2:  w = _message_player_2_widget;   break;
        case PLAYER_3:  w = _message_player_3_widget;   break;
    }

    w->add_anim_key()
        .set_delay(0.2F)
        .set_duration(0.3F)
        .set_scale(Vector3(1.1F,1.1F,1.1F));

    w->add_anim_key()
        .set_duration(0.3F)
        .set_scale(Vector3(0.0F,0.0F,0.0F));
}

void KaiserGameController::animate_update_score (void)
{
    std::string us,them;
    
    //
    // Titles
    //
    
    if (_highest_bidder_bid) {
        std::string msg = "(";
        msg += MoreStrings::cast_to_string(_highest_bidder_bid->bid());
        
        if (_highest_bidder_bid->is_no()) {
            msg += "No";
        } else {
            switch (_highest_bidder_bid->trump()) {
                case SUIT_CLUB:     msg += "{CLUB}";    break;
                case SUIT_DIAMOND:  msg += "{DIAMOND}"; break;
                case SUIT_HEART:    msg += "{HEART}";   break;
                case SUIT_SPADE:    msg += "{SPADE}";   break;
                default:
                    ASSERT(0);
            }
        }
        
        msg += ")";
        
        if (_highest_bidder == PLAYER_0 || _highest_bidder == PLAYER_2) {
            us = "{TXT_US}" + msg;
            them = "{TXT_THEM}";
        
        } else {
            us = "{TXT_US}";
            them = "{TXT_THEM}" + msg;
            
        }
        
    } else {
        us = "{TXT_US}";
        them = "{TXT_THEM}";
    }
    
    // Animate us
    if (_us_widget_title->label() != us) {
        _us_widget_title->add_anim_key()
            .set_duration(0.3F)
            .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        _us_widget_title->add_anim_key()
            .set_call(make_latent_call(this, &KaiserGameController::play_sound_click))
            .set_label(us)
            .set_duration(0.3F)
            .set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
    }
    
    // Animate them
    if (_them_widget_title->label() != them) {
        _them_widget_title->add_anim_key()
            .set_duration(0.3F)
            .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        _them_widget_title->add_anim_key()
            .set_call(make_latent_call(this, &KaiserGameController::play_sound_click))
            .set_label(them)
            .set_duration(0.3F)
            .set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
    }


    
    //
    // Score
    //
    
    if (_us_score_round != 0) {
        us = "{FMT_TEXT_SMALL}" + MoreStrings::cast_to_string(_us_score) + " {FMT_TEXT_SMALL_RED}(" + MoreStrings::cast_to_string(_us_score_round) + ")";
    } else {
        us = "{FMT_TEXT_SMALL}" + MoreStrings::cast_to_string(_us_score);
    }

    if (_them_score_round != 0) {
        them = "{FMT_TEXT_SMALL}" + MoreStrings::cast_to_string(_them_score) + " {FMT_TEXT_SMALL_RED}(" + MoreStrings::cast_to_string(_them_score_round) + ")";
    } else {
        them = "{FMT_TEXT_SMALL}" + MoreStrings::cast_to_string(_them_score);
    }
    
    // Animate us
    if (_us_widget->label() != us) {
        _us_widget->add_anim_key()
            .set_duration(0.3F)
            .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        _us_widget->add_anim_key()
            .set_call(make_latent_call(this, &KaiserGameController::play_sound_click))
            .set_label(us)
            .set_duration(0.3F)
            .set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
    }
    
    // Animate them
    if (_them_widget->label() != them) {
        _them_widget->add_anim_key()
            .set_duration(0.3F)
            .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        _them_widget->add_anim_key()
            .set_call(make_latent_call(this, &KaiserGameController::play_sound_click))
            .set_label(them)
            .set_duration(0.3F)
            .set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
    }
    
}

void KaiserGameController::animate_win (void)
{
    show_end_game ("{TXT_WIN}");

    LOG_MESSAGE << "Win!";

    // Update total score
    DTushort score = MoreStrings::cast_from_string<DTushort>(Globals::global("APP_USER_SCORE_WINS"));
    score += 1;

    Globals::set_global("APP_USER_SCORE_WINS", MoreStrings::cast_to_string(score), Globals::PERSISTENT_OBFUSCATED);
    Globals::save();


    if (!_human_player) {
        System::application()->transition_to_world(FilePath("{kaiser.lvl}"), "TransitionFadeOutIn", 1.0F, NULL, NULL);
    }
}

void KaiserGameController::animate_lose (void)
{
    show_end_game ("{TXT_LOSE}");

    LOG_MESSAGE << "Lose!";

    // Update total score
    DTushort score = MoreStrings::cast_from_string<DTushort>(Globals::global("APP_USER_SCORE_LOSSES"));
    score += 1;

    Globals::set_global("APP_USER_SCORE_LOSSES", MoreStrings::cast_to_string(score), Globals::PERSISTENT_OBFUSCATED);
    Globals::save();

    if (!_human_player) {
        System::application()->transition_to_world(FilePath("{kaiser.lvl}"), "TransitionFadeOutIn", 1.0F, NULL, NULL);
    }
}

void KaiserGameController::animate_tie (void)
{
    show_end_game ("{TXT_TIE}");

    LOG_MESSAGE << "Tie!";

    if (!_human_player) {
        System::application()->transition_to_world(FilePath("{kaiser.lvl}"), "TransitionFadeOutIn", 1.0F, NULL, NULL);
    }
}

//==============================================================================
//==============================================================================

void KaiserGameController::random_fade_in (const std::shared_ptr<GUILayout> &layout)
{
    if (!layout)    return;

    std::list<std::shared_ptr<GUIObject>> objects;
    objects = layout->all_objects ();
    
    FOR_EACH(i,objects) {
        (*i)->add_anim_key()
            .set_delay(MoreMath::random_float() * 0.2F)
            .set_duration(0.2F)
            .set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
    }
}

void KaiserGameController::random_fade_out (const std::shared_ptr<GUILayout> &layout)
{
    if (!layout)    return;

    std::list<std::shared_ptr<GUIObject>> objects;
    objects = layout->all_objects ();
    
    FOR_EACH(i,objects) {
        (*i)->add_anim_key()
            .set_delay(MoreMath::random_float() * 0.2F)
            .set_duration(0.2F)
            .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
    }
}

//==============================================================================
// __  __ _         ____  _     _
//|  \/  (_)       |  _ \(_)   | |
//| \  / |_ _ __   | |_) |_  __| |
//| |\/| | | '_ \  |  _ <| |/ _` |
//| |  | | | | | | | |_) | | (_| |
//|_|  |_|_|_| |_| |____/|_|\__,_|
//                            
//==============================================================================

void KaiserGameController::sync_options (void)
{
    _options_min_bid_5_widget->set_state(GUIObject::STATE_NORMAL);
    _options_min_bid_6_widget->set_state(GUIObject::STATE_NORMAL);
    _options_min_bid_7_widget->set_state(GUIObject::STATE_NORMAL);
    _options_min_bid_8_widget->set_state(GUIObject::STATE_NORMAL);

    switch (_minimum_bid) {
        case 5:     _options_min_bid_5_widget->set_state(GUIObject::STATE_FOCUSED);     break;
        case 6:     _options_min_bid_6_widget->set_state(GUIObject::STATE_FOCUSED);     break;
        case 7:     _options_min_bid_7_widget->set_state(GUIObject::STATE_FOCUSED);     break;
        case 8:     _options_min_bid_8_widget->set_state(GUIObject::STATE_FOCUSED);     break;
    }

    if (_option_no_trump_bidout)
        _options_no_trump_bidout_widget->set_label("{TXT_62}");
    else
        _options_no_trump_bidout_widget->set_label("{TXT_52}");

    if (_option_pass_cards)
        _options_pass_cards_widget->set_label("{TXT_ON}");
    else
        _options_pass_cards_widget->set_label("{TXT_OFF}");

    if (_option_game_over_52)
        _options_game_over_52_widget->set_label("{TXT_ON}");
    else
        _options_game_over_52_widget->set_label("{TXT_OFF}");

//    if (_option_steal_5_is_win)
//        _options_steal_5_to_win_widget->set_label("{TXT_ON}");
//    else
//        _options_steal_5_to_win_widget->set_label("{TXT_OFF}");
}

void KaiserGameController::show_options_min_bidding (void)
{
    sync_options();

    random_fade_in(_gui_layout_options_min_bidding);

    _options_set = false;
}

void KaiserGameController::switch_options_min_bid (DTshort bid)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    _minimum_bid = bid;
    sync_options();

    LOG_MESSAGE << "switch_options_min_bid called";
}

void KaiserGameController::switch_options_no_trump_bidout (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    _option_no_trump_bidout = !_option_no_trump_bidout;
    sync_options();
}

void KaiserGameController::switch_options_pass_cards (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    _option_pass_cards = !_option_pass_cards;
    sync_options();
}

void KaiserGameController::switch_options_game_over_52 (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    _option_game_over_52 = !_option_game_over_52;
    sync_options();
}

//void KaiserGameController::switch_options_steal_5_is_win (void)
//{
//    if (_pause_sound_on)
//        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());
//
//    _option_steal_5_is_win = !_option_steal_5_is_win;
//    sync_options();
//}

void KaiserGameController::accept_options_min_bid (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    Globals::set_global("APP_MIN_BID", MoreStrings::cast_to_string(_minimum_bid), Globals::PERSISTENT);
    Globals::set_global("APP_NO_TRUMP_BIDOUT", MoreStrings::cast_to_string(_option_no_trump_bidout), Globals::PERSISTENT);
    Globals::set_global("APP_PASS_CARDS", MoreStrings::cast_to_string(_option_pass_cards), Globals::PERSISTENT);
    Globals::set_global("APP_GAME_OVER_52", MoreStrings::cast_to_string(_option_game_over_52), Globals::PERSISTENT);
//    Globals::set_global("APP_STEAL_5_TO_WIN", MoreStrings::cast_to_string(_option_steal_5_is_win), Globals::PERSISTENT);

    random_fade_out(_gui_layout_options_min_bidding);
    LOG_MESSAGE << "accept_bid called";

    _options_set = true;
}

//==============================================================================
//==============================================================================

void KaiserGameController::switch_pause_sound (void)
{
    _pause_sound_on = !_pause_sound_on;

    if (_pause_sound_on)    _pause_sound->set_label("{TXT_SOUND_ON}");
    else                    _pause_sound->set_label("{TXT_SOUND_OFF}");

    Globals::set_global("APP_SOUND_ON", MoreStrings::cast_to_string(_pause_sound_on), Globals::PERSISTENT);

    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());
}

//==============================================================================
// ____  _     _     _ _
//|  _ \(_)   | |   | (_)            
//| |_) |_  __| | __| |_ _ __   __ _ 
//|  _ <| |/ _` |/ _` | | '_ \ / _` |
//| |_) | | (_| | (_| | | | | | (_| |
//|____/|_|\__,_|\__,_|_|_| |_|\__, |
//                              __/ |
//                             |___/ 
//
//==============================================================================

void KaiserGameController::update_bidding (void)
{
    if (_highest_bidder_bid) {
        DTshort bid =   (_bid_5_widget->state() == GUIObject::STATE_FOCUSED) * 5 +
                        (_bid_6_widget->state() == GUIObject::STATE_FOCUSED) * 6 +
                        (_bid_7_widget->state() == GUIObject::STATE_FOCUSED) * 7 +
                        (_bid_8_widget->state() == GUIObject::STATE_FOCUSED) * 8 +
                        (_bid_9_widget->state() == GUIObject::STATE_FOCUSED) * 9 +
                        (_bid_10_widget->state() == GUIObject::STATE_FOCUSED) * 10 +
                        (_bid_11_widget->state() == GUIObject::STATE_FOCUSED) * 11 +
                        (_bid_12_widget->state() == GUIObject::STATE_FOCUSED) * 12;
        
        DTboolean no = _bid_no_widget->state();

        DTshort this_bid = (bid << 1) | no;
        DTshort top_bid = (_highest_bidder_bid->bid() << 1) | _highest_bidder_bid->is_no();

        if (_dealer == PLAYER_0) {
            if (this_bid >= top_bid)    _bid_bid_widget->set_state(GUIObject::STATE_NORMAL);
            else                        _bid_bid_widget->set_state(GUIObject::STATE_DISABLED);
        } else {
            if (this_bid > top_bid)     _bid_bid_widget->set_state(GUIObject::STATE_NORMAL);
            else                        _bid_bid_widget->set_state(GUIObject::STATE_DISABLED);
        }

    } else {
        _bid_bid_widget->set_state(GUIObject::STATE_NORMAL);
    }

}

void KaiserGameController::show_bidding (void)
{

    if (_minimum_bid > 5)   _bid_5_widget->set_state(GUIObject::STATE_DISABLED);
    else                    _bid_5_widget->set_state(GUIObject::STATE_NORMAL);

    if (_minimum_bid > 6)   _bid_6_widget->set_state(GUIObject::STATE_DISABLED);
    else                    _bid_6_widget->set_state(GUIObject::STATE_NORMAL);

    if (_minimum_bid > 7)   _bid_7_widget->set_state(GUIObject::STATE_DISABLED);
    else                    _bid_7_widget->set_state(GUIObject::STATE_NORMAL);

    _bid_8_widget->set_state(GUIObject::STATE_NORMAL);
    _bid_9_widget->set_state(GUIObject::STATE_NORMAL);
    _bid_10_widget->set_state(GUIObject::STATE_NORMAL);
    _bid_11_widget->set_state(GUIObject::STATE_NORMAL);
    _bid_12_widget->set_state(GUIObject::STATE_NORMAL);

    _bid_no_widget->set_state(GUIObject::STATE_NORMAL);

    _bid_bid_widget->set_state(GUIObject::STATE_NORMAL);
    _bid_pass_widget->set_state(GUIObject::STATE_NORMAL);

    if (_minimum_bid > 7)       _bid_8_widget->set_state(GUIObject::STATE_FOCUSED);
    else if (_minimum_bid > 6)  _bid_7_widget->set_state(GUIObject::STATE_FOCUSED);
    else if (_minimum_bid > 5)  _bid_6_widget->set_state(GUIObject::STATE_FOCUSED);
    else                        _bid_5_widget->set_state(GUIObject::STATE_FOCUSED);

    update_bidding();

    random_fade_in(_gui_layout_bidding);
}

void KaiserGameController::switch_bid (DTshort bid)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    if (bid != 0) {

        if (_bid_5_widget->state() != GUIObject::STATE_DISABLED)
            _bid_5_widget->set_state(GUIObject::STATE_NORMAL);

        if (_bid_6_widget->state() != GUIObject::STATE_DISABLED)
            _bid_6_widget->set_state(GUIObject::STATE_NORMAL);

        if (_bid_7_widget->state() != GUIObject::STATE_DISABLED)
            _bid_7_widget->set_state(GUIObject::STATE_NORMAL);

        _bid_8_widget->set_state(GUIObject::STATE_NORMAL);
        _bid_9_widget->set_state(GUIObject::STATE_NORMAL);
        _bid_10_widget->set_state(GUIObject::STATE_NORMAL);
        _bid_11_widget->set_state(GUIObject::STATE_NORMAL);
        _bid_12_widget->set_state(GUIObject::STATE_NORMAL);

        switch (bid) {
            case 5:     _bid_5_widget->set_state(GUIObject::STATE_FOCUSED);     break;
            case 6:     _bid_6_widget->set_state(GUIObject::STATE_FOCUSED);     break;
            case 7:     _bid_7_widget->set_state(GUIObject::STATE_FOCUSED);     break;
            case 8:     _bid_8_widget->set_state(GUIObject::STATE_FOCUSED);     break;
            case 9:     _bid_9_widget->set_state(GUIObject::STATE_FOCUSED);     break;
            case 10:    _bid_10_widget->set_state(GUIObject::STATE_FOCUSED);    break;
            case 11:    _bid_11_widget->set_state(GUIObject::STATE_FOCUSED);    break;
            case 12:    _bid_12_widget->set_state(GUIObject::STATE_FOCUSED);    break;
        }

    }

    update_bidding();

    LOG_MESSAGE << "switch_bid called";
}

void KaiserGameController::pass_bid (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    _bid = std::shared_ptr<KaiserBid>(new KaiserBid() );

    random_fade_out(_gui_layout_bidding);
    LOG_MESSAGE << "pass_bid called";
}

void KaiserGameController::accept_bid (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    DTshort bid =   (_bid_5_widget->state() == GUIObject::STATE_FOCUSED) * 5 +
                    (_bid_6_widget->state() == GUIObject::STATE_FOCUSED) * 6 +
                    (_bid_7_widget->state() == GUIObject::STATE_FOCUSED) * 7 +
                    (_bid_8_widget->state() == GUIObject::STATE_FOCUSED) * 8 +
                    (_bid_9_widget->state() == GUIObject::STATE_FOCUSED) * 9 +
                    (_bid_10_widget->state() == GUIObject::STATE_FOCUSED) * 10 +
                    (_bid_11_widget->state() == GUIObject::STATE_FOCUSED) * 11 +
                    (_bid_12_widget->state() == GUIObject::STATE_FOCUSED) * 12;
    
    DTboolean no = _bid_no_widget->state();;

    _bid = std::shared_ptr<KaiserBid>(new KaiserBid(bid,no) );

    random_fade_out(_gui_layout_bidding);
    LOG_MESSAGE << "accept_bid called";
}

//==============================================================================
// _______
//|__   __|                        
//   | |_ __ _   _ _ __ ___  _ __  
//   | | '__| | | | '_ ` _ \| '_ \ 
//   | | |  | |_| | | | | | | |_) |
//   |_|_|   \__,_|_| |_| |_| .__/ 
//                          | |    
//                          |_|    
//==============================================================================

void KaiserGameController::show_trump (DTboolean force)
{
    _trump_club_widget->set_state(GUIObject::STATE_NORMAL);
    _trump_diamond_widget->set_state(GUIObject::STATE_NORMAL);
    _trump_heart_widget->set_state(GUIObject::STATE_NORMAL);
    _trump_spade_widget->set_state(GUIObject::STATE_NORMAL);
    _trump_no_widget->set_state(force ? GUIObject::STATE_NORMAL : GUIObject::STATE_DISABLED);
    _trump_go_widget->set_state(GUIObject::STATE_DISABLED);

    random_fade_in(_gui_layout_declare_trump);
}


void KaiserGameController::switch_trump (Suit trump)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    _trump_club_widget->set_state(GUIObject::STATE_NORMAL);
    _trump_diamond_widget->set_state(GUIObject::STATE_NORMAL);
    _trump_heart_widget->set_state(GUIObject::STATE_NORMAL);
    _trump_spade_widget->set_state(GUIObject::STATE_NORMAL);

    if (_trump_no_widget->state() != GUIObject::STATE_DISABLED)
        _trump_no_widget->set_state(GUIObject::STATE_NORMAL);

    switch (trump) {
        case SUIT_CLUB:     _trump_club_widget->set_state(GUIObject::STATE_FOCUSED);        break;
        case SUIT_DIAMOND:  _trump_diamond_widget->set_state(GUIObject::STATE_FOCUSED);     break;
        case SUIT_HEART:    _trump_heart_widget->set_state(GUIObject::STATE_FOCUSED);       break;
        case SUIT_SPADE:    _trump_spade_widget->set_state(GUIObject::STATE_FOCUSED);       break;
        case SUIT_NO:       _trump_no_widget->set_state(GUIObject::STATE_FOCUSED);          break;
        default:
            break;
    }
    
    _trump_go_widget->set_state(GUIObject::STATE_NORMAL);
    
    LOG_MESSAGE << "switch_trump called";
}

void KaiserGameController::accept_trump (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());

    if (_trump_no_widget->state() == GUIObject::STATE_FOCUSED) {
        _highest_bidder_bid->set_trump(SUIT_UNDEFINED);
        _highest_bidder_bid->set_no(true);

    } else {

        Suit trump = (Suit)  (  _trump_club_widget->state() * SUIT_CLUB +
                                _trump_diamond_widget->state() * SUIT_DIAMOND +
                                _trump_heart_widget->state() * SUIT_HEART +
                                _trump_spade_widget->state() * SUIT_SPADE
                            );

        _highest_bidder_bid->set_trump(trump);
        _highest_bidder_bid->set_no(false);
    }

    random_fade_out(_gui_layout_declare_trump);
    LOG_MESSAGE << "accept_trump called";
}

//==============================================================================
// _____                     
//|  __ \                    
//| |__) |_ _ _   _ ___  ___ 
//|  ___/ _` | | | / __|/ _ \
//| |  | (_| | |_| \__ \  __/
//|_|   \__,_|\__,_|___/\___|
//        
//==============================================================================

void KaiserGameController::show_pause (void)
{
    random_fade_in(_gui_pause);
}

//==============================================================================
// _____  _                                 _ 
//|  __ \| |                               | |
//| |__) | | __ _ _   _    ___ __ _ _ __ __| |
//|  ___/| |/ _` | | | |  / __/ _` | '__/ _` |
//| |    | | (_| | |_| | | (_| (_| | | | (_| |
//|_|    |_|\__,_|\__, |  \___\__,_|_|  \__,_|
//                 __/ |                      
//                |___/                       
//
//==============================================================================

void KaiserGameController::play_card (std::shared_ptr<KaiserObjectCard> card)
{
    _played_cards[_player_turn] = card;
    //_played_cards[_player_turn]->set_can_interact(false);
}

//==============================================================================
// _____                 _____              _
//|  __ \               / ____|            | |
//| |__) |_ _ ___ ___  | |     __ _ _ __ __| |
//|  ___/ _` / __/ __| | |    / _` | '__/ _` |
//| |  | (_| \__ \__ \ | |___| (_| | | | (_| |
//|_|   \__,_|___/___/  \_____\__,_|_|  \__,_|
//==============================================================================


void KaiserGameController::pass_card (std::shared_ptr<KaiserObjectCard> card)
{

    // Shuffle animation first
    auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(card),
                                        &PlaceableObject::transform,
                                        &PlaceableObject::set_transform);

    h->append(pass_card_transform(PLAYER_0), 0.5F, 0.0F, std::make_shared<PropertyAnimatorCard>());

    play_sound_deal();
    _passed_cards.push_back(card);

    card->set_can_interact(false);
}

//==============================================================================
//==============================================================================

void KaiserGameController::trick_card (std::shared_ptr<KaiserObjectCard> card, DTboolean showing)
{


    for (DTshort p = 0; p < 4; ++p) {

        std::vector<KaiserPlayer::Trick>& tricks = _players[p].tricks();

        for (DTshort i = 0; i < tricks.size(); ++i) {
            KaiserPlayer::Trick& trick = tricks[i];

            for (DTint j = 0; j < 4; ++j) {

                // Check if this trick has the card in question
                if (trick._played_cards[j] == card) {

                    // Animate every card
                    for (DTint k = 0; k < 4; ++k) {
                        auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(trick._played_cards[k]),
                                                            &PlaceableObject::transform,
                                                            &PlaceableObject::set_transform);

                        if (showing) {
                            h->append(played_card_transform( (Player) k), 0.2F, MoreMath::random_float() * 0.2F, std::make_shared<PropertyAnimatorCard>());

                        } else {
                            h->append(trick_card_transform( (Player) p, i), 0.2F, MoreMath::random_float() * 0.2F, std::make_shared<PropertyAnimatorCard>());
                        }

                    } // animate cards

                    return;
                }

            } // scan cards
            
        } // for tricks

    }


}

//==============================================================================
// ______           _    ____   __    _____
//|  ____|         | |  / __ \ / _|  / ____|                     
//| |__   _ __   __| | | |  | | |_  | |  __  __ _ _ __ ___   ___ 
//|  __| | '_ \ / _` | | |  | |  _| | | |_ |/ _` | '_ ` _ \ / _ \
//| |____| | | | (_| | | |__| | |   | |__| | (_| | | | | | |  __/
//|______|_| |_|\__,_|  \____/|_|    \_____|\__,_|_| |_| |_|\___|
//
//==============================================================================

void KaiserGameController::show_end_game (const std::string &msg)
{
    _gui_end_of_game_message->set_label(msg);
    random_fade_in(_gui_end_of_game);
}

void KaiserGameController::restart (void)
{
    System::application()->transition_to_world(FilePath("{kaiser.lvl}"), "TransitionFadeOutIn", 1.0F, NULL, NULL);
}

void KaiserGameController::quit (void)
{
    random_fade_out(_gui_pause);
    random_fade_in(_enter_high_score_layout);
}

void KaiserGameController::resume (void)
{
    random_fade_out(_gui_pause);
}

//==============================================================================
// _    _ _       _        _____
//| |  | (_)     | |      / ____|                   
//| |__| |_  __ _| |__   | (___   ___ ___  _ __ ___ 
//|  __  | |/ _` | '_ \   \___ \ / __/ _ \| '__/ _ \
//| |  | | | (_| | | | |  ____) | (_| (_) | | |  __/
//|_|  |_|_|\__, |_| |_| |_____/ \___\___/|_|  \___|
//           __/ |                                  
//          |___/
//==============================================================================

void KaiserGameController::click_key (std::string k)
{
    std::string label = Globals::global("APP_USER_NAME");
    
    if (label == "KaiserDude") {
        label.clear();
    }
    
    if (k == "<") {
        play_sound_click();

        if (label.size() > 0)
            label = label.substr(0,label.size()-1);
        
    } else {
        play_sound_click();

        // Replace last character
        if (label.size() >= 32)
            label = label.substr(0,31);

        label += k;
    }
    
    Globals::set_global("APP_USER_NAME", label, Globals::PERSISTENT);
}

void KaiserGameController::submit (void)
{
    random_fade_out(_enter_high_score_layout);
    
    if (!_submitted) {
        _submitted = true;

        DTPortal::HighScore s;
        s.name = Globals::global("APP_USER_NAME");

        DTshort wins = MoreStrings::cast_from_string<DTshort>(Globals::global("APP_USER_SCORE_WINS"));
        DTshort losses = MoreStrings::cast_from_string<DTshort>(Globals::global("APP_USER_SCORE_LOSSES"));

        s.score = (wins << 16) | losses;

        DTPortal::submit_high_score("Kaiser", s, NULL, NULL);
    }
    
    System::application()->transition_to_world(FilePath("{title.lvl}"), "TransitionFadeOutIn", 1.0F, NULL, NULL);
}

void KaiserGameController::skip (void)
{
    random_fade_out(_enter_high_score_layout);
    
    System::application()->transition_to_world(FilePath("{title.lvl}"), "TransitionFadeOutIn", 1.0F, NULL, NULL);
}

//==============================================================================
//==============================================================================

void KaiserGameController::go_to_store_nag (void)
{
    random_fade_out(_get_full_version_layout);

    System::application()->transition_to_world(FilePath("{title.lvl}"), "TransitionFadeOutIn", 1.0F, NULL, NULL, "launch_store");
}

void KaiserGameController::show_nag (void)
{
    _dismissed_nag = false;
    random_fade_in(_get_full_version_layout);
}

void KaiserGameController::dismiss_nag (void)
{
    _dismissed_nag = true;
    random_fade_out(_get_full_version_layout);
}

//==============================================================================
//==============================================================================

void KaiserGameController::click_facebook (void)
{
    show_facebook (Globals::global("TXT_FACEBOOK_MSG"));
}

void KaiserGameController::click_twitter (void)
{
    show_twitter (Globals::global("TXT_TWITTER_MSG"));
}

//==============================================================================
//==============================================================================

std::shared_ptr<KaiserObjectCard> KaiserGameController::get_card (Card c, Suit s)
{
    for (DTint i = 0; i < _cards.size(); ++i) {

        if (_cards[i]->suit() == s && _cards[i]->value() == c)
            return _cards[i];
    }

    return NULL;
}

//==============================================================================
//==============================================================================

void KaiserGameController::run_co (Coroutine<KaiserGameController> *co)
{
    animate_update_score();
    
    DTfloat best_delay = _human_player ? 0.65F : 0.05F;
    DTfloat best_delay_variation = _human_player ? 0.5F : 0.0F;

    
    //
    // 0. Choose minimum bid (If full version)
    //
    
    Analytics::record_event("Kaiser", "GameState", "Start");
    
    if (MoreStrings::cast_from_string<DTboolean>(Globals::global("APP_FULL_VERSION"))) {
    
        if (_human_player) {
            _co.yield(2.0F);

            show_options_min_bidding();

            // Wait for bid
            while (!_options_set)
                _co.yield(0.0F);
        } else {
            _minimum_bid = 5;
        }
        
    } else {
        _options_set = true;
        _minimum_bid = 7;


        // Nag screen
        show_nag();
        while (!_dismissed_nag)
            _co.yield(0.0F);

        _co.yield(1.0F);
    }



    _interactions_component->set_trick_cb(make_callback(this, &KaiserGameController::trick_card));


    //
    // 1. Choose Dealer
    //
    
    LOG_MESSAGE << "Choose Dealer";
    
    MoreMath::set_random_seed();
    _dealer = (Player) (MoreMath::random_MT_int() % 4);


    // Do the rounds
    while (1) {

        // Wait for animations to get done
        while (PropertyAnimator::is_animating())
            _co.yield(0.0F);

        _dealer = (Player) ((_dealer + 1) % 4);
#if FORCE_DEAL
        _dealer = PLAYER_0;
#endif

        animate_dealer_turn (_dealer);
        animate_player_turn (_dealer);

        //
        // 2. Dealing
        //
        
        LOG_MESSAGE << "Dealing";
        
        // Clear player state
        for (DTshort p = 0; p < 4; ++p) {
            _ai[p].begin((Player) p);
            _players[p].clear();
        }

        random_fade_out(_gui_layout_bidding);
        random_fade_out(_gui_layout_declare_trump);
    
        // Clear bids
        _highest_bidder_bid.reset();

        // Shuffle the cards
        _deck = _cards;
        
        MoreMath::set_random_seed();
        std::random_shuffle(_deck.begin(), _deck.end(), [](DTint n) { return MoreMath::random_MT_int() % n; } );


#if FORCE_DEAL

//    CARD_9, SUIT_SPADE
//    CARD_10, SUIT_HEART
//    CARD_8, SUIT_HEART
//    CARD_QUEEN, SUIT_CLUB
//    CARD_9, SUIT_CLUB
//    CARD_7, SUIT_CLUB
//    CARD_QUEEN, SUIT_DIAMOND
//    CARD_8, SUIT_CLUB

//    CARD_10, SUIT_DIAMOND
//    CARD_KING, SUIT_SPADE
//    CARD_JACK, SUIT_SPADE
//    CARD_10, SUIT_SPADE
//    CARD_8, SUIT_SPADE
//    CARD_QUEEN, SUIT_SPADE
//    CARD_KING, SUIT_DIAMOND
//    CARD_JACK, SUIT_DIAMOND

//    CARD_ACE, SUIT_CLUB
//    CARD_KING, SUIT_CLUB
//    CARD_JACK, SUIT_CLUB
//    CARD_10, SUIT_CLUB
//    CARD_7, SUIT_DIAMOND
//    CARD_3, SUIT_SPADE
//    CARD_5, SUIT_HEART
//    CARD_JACK, SUIT_HEART

//    CARD_ACE, SUIT_SPADE
//    CARD_ACE, SUIT_DIAMOND
//    CARD_QUEEN, SUIT_HEART
//    CARD_9, SUIT_HEART
//    CARD_8, SUIT_DIAMOND
//    CARD_9, SUIT_DIAMOND
//    CARD_ACE, SUIT_HEART
//    CARD_KING, SUIT_HEART


        // Player 0

        _deck[0+0] = get_card (CARD_9, SUIT_SPADE);
        _deck[0+4] = get_card (CARD_10, SUIT_HEART);
        _deck[0+8] = get_card (CARD_8, SUIT_HEART);
        _deck[0+12] = get_card (CARD_QUEEN, SUIT_CLUB);
        _deck[0+16] = get_card (CARD_9, SUIT_CLUB);
        _deck[0+20] = get_card (CARD_7, SUIT_CLUB);
        _deck[0+24] = get_card (CARD_QUEEN, SUIT_DIAMOND);
        _deck[0+28] = get_card (CARD_8, SUIT_CLUB);

        // Player 1
        _deck[1+0] = get_card (CARD_ACE, SUIT_SPADE);
        _deck[1+4] = get_card (CARD_ACE, SUIT_DIAMOND);
        _deck[1+8] = get_card (CARD_JACK, SUIT_SPADE);
        _deck[1+12] = get_card (CARD_9, SUIT_HEART);
        _deck[1+16] = get_card (CARD_8, SUIT_DIAMOND);
        _deck[1+20] = get_card (CARD_9, SUIT_DIAMOND);
        _deck[1+24] = get_card (CARD_ACE, SUIT_HEART);
        _deck[1+28] = get_card (CARD_KING, SUIT_HEART);

        // Player 2
        _deck[2+0] = get_card (CARD_ACE, SUIT_CLUB);
        _deck[2+4] = get_card (CARD_KING, SUIT_CLUB);
        _deck[2+8] = get_card (CARD_JACK, SUIT_CLUB);
        _deck[2+12] = get_card (CARD_10, SUIT_CLUB);
        _deck[2+16] = get_card (CARD_7, SUIT_DIAMOND);
        _deck[2+20] = get_card (CARD_3, SUIT_SPADE);
        _deck[2+24] = get_card (CARD_5, SUIT_HEART);
        _deck[2+28] = get_card (CARD_JACK, SUIT_HEART);

        // Player 3
        _deck[3+0] = get_card (CARD_10, SUIT_DIAMOND);
        _deck[3+4] = get_card (CARD_KING, SUIT_SPADE);
        _deck[3+8] = get_card (CARD_QUEEN, SUIT_HEART);
        _deck[3+12] = get_card (CARD_10, SUIT_SPADE);
        _deck[3+16] = get_card (CARD_8, SUIT_SPADE);
        _deck[3+20] = get_card (CARD_QUEEN, SUIT_SPADE);
        _deck[3+24] = get_card (CARD_KING, SUIT_DIAMOND);
        _deck[3+28] = get_card (CARD_JACK, SUIT_DIAMOND);

#endif
        
        // Deal
        DTfloat deal_delay = 0.0F;
        DTfloat shuffle_delay = 0.0F;
        DTint cc = 0;
        
        for (DTint c = 0; c < 8; ++c) {
        
            for (DTint dp = 0; dp < 4; ++dp) {
                DTint dpp = (_dealer + dp + 1) % 4;
                
                std::shared_ptr<KaiserObjectCard> card = _deck.back();
                
                card->set_can_use(true);
                card->set_trick(false);

                _players[dpp].add_card(card);
                _deck.pop_back();
                
                // Allow interactions for player cards
                card->set_can_interact( (dpp == PLAYER_0) && _human_player);
            
                // Shuffle animation first
                auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(card),
                                                    &PlaceableObject::transform,
                                                    &PlaceableObject::set_transform);

                // Animate to center
                DTfloat time = 0.2F + MoreMath::random_MT_float() * 0.2F;
                
                Matrix4 t0 = Matrix4(flip_over(Matrix3::identity()), Vector3(0.0F,0.0F,0.0F));
                h->append(t0, time, 0.0F, std::make_shared<PropertyAnimatorCard>());
                
                // Animate into two piles
                Matrix4 t1 =    (cc % 2) ?
                                Matrix4(flip_over(Matrix3::set_rotation_y(-0.2F) * Matrix3::set_rotation_z(0.2F)), Vector3(-1.0F,0.0F,0.0F)) :
                                Matrix4(flip_over(Matrix3::set_rotation_y(0.2F) * Matrix3::set_rotation_z(-0.2F)), Vector3(1.0F,0.0F,0.0F));
                h->append(t1, 0.2F, 0.4F - time, std::make_shared<PropertyAnimatorCard>());
                
                // Add shuffle sound for first card
                if (c == 0 && dp == 0)
                    h->append(0.0F, make_latent_call(this,&KaiserGameController::play_sound_shuffle));

                // Animate into a single pile again
                Matrix4 t3 = Matrix4(flip_over(Matrix3::identity()), Vector3(0.0F,0.0F,0.0F));
                h->append(t3, 0.4F, shuffle_delay, std::make_shared<PropertyAnimatorCard>());
                shuffle_delay += 0.02F;

                // Animate into a single pile again
                Matrix4 t4 = dealer_card_transform(_dealer);
                h->append(t4, 0.5F, 0.8F - shuffle_delay, std::make_shared<PropertyAnimatorCard>());
                
                // Deal animation
                Matrix4 t5 = player_card_transform ( (Player) dpp, c, 8);
                h->append(t5, 0.5F, deal_delay + 0.1F, std::make_shared<PropertyAnimatorCard>());
                deal_delay += 0.05F;
                
                // Deal sound
                if (dp == 0)
                    h->append(0.0F, make_latent_call(this,&KaiserGameController::play_sound_deal));
                
                ++cc;
            }
            
        }

        // Output cards just in case we need to replay
        for (DTint dp = 0; dp < 4; ++dp) {
            const std::vector<std::shared_ptr<KaiserObjectCard>> &cards = _players->cards();

            for (DTint c = 0; c < cards.size(); ++c) {
                LOG_MESSAGE << "_deck[" << dp << "+" << c << "] = get_card ( (Card)" << cards[c]->value() << ", (Suit)" << cards[c]->suit() << ");";
            }
        }

        // Wait for animations to get done
        while (PropertyAnimator::is_animating())
            _co.yield(0.0F);

        // Sort player cards by suit and value
        _players[PLAYER_0].sort();

        // Animate
        const std::vector<std::shared_ptr<KaiserObjectCard>> &sorted = _players[PLAYER_0].cards();
        DTfloat sort_delay = 0.0F;

        for (DTsize c = 0; c < sorted.size(); ++c) {
            // Shuffle animation first
            auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(sorted[c]),
                                                &PlaceableObject::transform,
                                                &PlaceableObject::set_transform);

            Matrix4 t = player_card_transform ( PLAYER_0, c, 8);
            h->append(t, 0.3F, sort_delay, std::make_shared<PropertyAnimatorCardUp>());

            sort_delay += 0.1F;
        }

        _co.yield(1.0F);


        //
        // 3 Pass cards
        //

        // Wait for animations to get done
        while (PropertyAnimator::is_animating())
            _co.yield(0.0F);

        for (auto &i : _cards) {
            i->save_transform();
        }

        if (_option_pass_cards) {
            animate_player_turn (PLAYER_0);

            // Allow player to interact with card
            _interactions_component->set_allow_interactions(true);

            _interactions_component->set_played_cb(make_callback(this, &KaiserGameController::pass_card));

            std::shared_ptr<KaiserObjectCard> c0_p0;
            std::shared_ptr<KaiserObjectCard> c1_p0;
            std::shared_ptr<KaiserObjectCard> c0_p1;
            std::shared_ptr<KaiserObjectCard> c1_p1;
            std::shared_ptr<KaiserObjectCard> c0_p2;
            std::shared_ptr<KaiserObjectCard> c1_p2;
            std::shared_ptr<KaiserObjectCard> c0_p3;
            std::shared_ptr<KaiserObjectCard> c1_p3;

            // Animate computer passed cards
            if (!_human_player) {
                _ai[PLAYER_0].pass(PLAYER_0, _players[PLAYER_0].cards(), c0_p0, c1_p0);
                auto hc0_p0 = PropertyAnimator::animate( checked_cast<PlaceableObject>(c0_p0), &PlaceableObject::transform, &PlaceableObject::set_transform);
                hc0_p0->append(pass_card_transform(PLAYER_0), 0.5F, MoreMath::random_float(), std::make_shared<PropertyAnimatorCard>());
                auto hc1_p0 = PropertyAnimator::animate( checked_cast<PlaceableObject>(c1_p0), &PlaceableObject::transform, &PlaceableObject::set_transform);
                hc1_p0->append(pass_card_transform(PLAYER_0), 0.5F, MoreMath::random_float(), std::make_shared<PropertyAnimatorCard>());
            }

            _ai[PLAYER_1].pass(PLAYER_1, _players[PLAYER_1].cards(), c0_p1, c1_p1);
            auto hc0_p1 = PropertyAnimator::animate( checked_cast<PlaceableObject>(c0_p1), &PlaceableObject::transform, &PlaceableObject::set_transform);
            hc0_p1->append(pass_card_transform(PLAYER_1), 0.5F, MoreMath::random_float(), std::make_shared<PropertyAnimatorCard>());
            auto hc1_p1 = PropertyAnimator::animate( checked_cast<PlaceableObject>(c1_p1), &PlaceableObject::transform, &PlaceableObject::set_transform);
            hc1_p1->append(pass_card_transform(PLAYER_1), 0.5F, MoreMath::random_float(), std::make_shared<PropertyAnimatorCard>());

            _ai[PLAYER_2].pass(PLAYER_2, _players[PLAYER_2].cards(), c0_p2, c1_p2);
            auto hc0_p2 = PropertyAnimator::animate( checked_cast<PlaceableObject>(c0_p2), &PlaceableObject::transform, &PlaceableObject::set_transform);
            hc0_p2->append(pass_card_transform(PLAYER_2), 0.5F, MoreMath::random_float(), std::make_shared<PropertyAnimatorCard>());
            auto hc1_p2 = PropertyAnimator::animate( checked_cast<PlaceableObject>(c1_p2), &PlaceableObject::transform, &PlaceableObject::set_transform);
            hc1_p2->append(pass_card_transform(PLAYER_2), 0.5F, MoreMath::random_float(), std::make_shared<PropertyAnimatorCard>());

            _ai[PLAYER_3].pass(PLAYER_3, _players[PLAYER_3].cards(), c0_p3, c1_p3);
            auto hc0_p3 = PropertyAnimator::animate( checked_cast<PlaceableObject>(c0_p3), &PlaceableObject::transform, &PlaceableObject::set_transform);
            hc0_p3->append(pass_card_transform(PLAYER_3), 0.5F, MoreMath::random_float(), std::make_shared<PropertyAnimatorCard>());
            auto hc1_p3 = PropertyAnimator::animate( checked_cast<PlaceableObject>(c1_p3), &PlaceableObject::transform, &PlaceableObject::set_transform);
            hc1_p3->append(pass_card_transform(PLAYER_3), 0.5F, MoreMath::random_float(), std::make_shared<PropertyAnimatorCard>());

            if (_human_player) {
                _passed_cards.clear();
                while (_passed_cards.size() < 2)
                    _co.yield(0.0F);

                // Disallow player to interact with card
                _interactions_component->set_allow_interactions(false);

                c0_p0 = _passed_cards[0];
                c1_p0 = _passed_cards[1];
            }

            _co.yield(1.0F);


            _players[PLAYER_0].swap(c0_p0, c0_p2);
            _players[PLAYER_0].swap(c1_p0, c1_p2);
            _players[PLAYER_2].swap(c0_p2, c0_p0);
            _players[PLAYER_2].swap(c1_p2, c1_p0);

            _players[PLAYER_1].swap(c0_p1, c0_p3);
            _players[PLAYER_1].swap(c1_p1, c1_p3);
            _players[PLAYER_3].swap(c0_p3, c0_p1);
            _players[PLAYER_3].swap(c1_p3, c1_p1);

            // Update interactive flags
            c0_p0->set_can_interact(false);
            c1_p0->set_can_interact(false);
            c0_p2->set_can_interact(_human_player);
            c1_p2->set_can_interact(_human_player);


            // Wait for animations to get done
            while (PropertyAnimator::is_animating())
                _co.yield(0.0F);

            // Animate cards into hands
            for (DTshort p = 0; p < 4; ++p) {
                const std::vector<std::shared_ptr<KaiserObjectCard>> &cards = _players[p].cards();

                for (DTshort c = 0; c < cards.size(); ++c) {
                    Matrix4 t = player_card_transform( (Player) p, c, 8);

                    if (t != cards[c]->transform()) {
                        auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(cards[c]), &PlaceableObject::transform, &PlaceableObject::set_transform);
                        h->append(t, 0.5F, MoreMath::random_float() * 0.3F, std::make_shared<PropertyAnimatorCard>());
                    }
                }
            }

            _co.yield(1.5F);

            // Wait for animations to get done
            while (PropertyAnimator::is_animating())
                _co.yield(0.0F);

            // Sort again
            _players[PLAYER_0].sort();

            // Animate
            const std::vector<std::shared_ptr<KaiserObjectCard>> &sorted = _players[PLAYER_0].cards();
            DTfloat sort_delay = 0.0F;

            for (DTsize c = 0; c < sorted.size(); ++c) {

                // Shuffle animation first
                auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(sorted[c]),
                                                    &PlaceableObject::transform,
                                                    &PlaceableObject::set_transform);

                Matrix4 t = player_card_transform ( PLAYER_0, c, 8);
                h->append(t, 0.3F, sort_delay, std::make_shared<PropertyAnimatorCardUp>());

                sort_delay += 0.1F;
            }

            _co.yield(1.0F);

        }

        // Wait for animations to get done
        while (PropertyAnimator::is_animating())
            _co.yield(0.0F);

        for (auto &i : _cards) {
            i->save_transform();
        }

        //
        // 4. Do bidding
        //
        
        LOG_MESSAGE << "Bidding";
        
        DTboolean forced_bid = false;
        
        for (DTshort p = 0; p < 4; ++p) {
            _player_turn = (Player)((_dealer + p + 1) % 4);
            DTboolean is_dealer = (_player_turn == _dealer);
            
            // RULE: If no players bid then the dealer must make a "forced bid" for the set
            // minimum bid (although he can pick any suit or no-trump).
            forced_bid |= is_dealer && _highest_bidder_bid && _highest_bidder_bid->is_pass();

            animate_player_turn (_player_turn);
            
            _co.yield(best_delay + (MoreMath::random_float() - 0.5F) * best_delay_variation);

            _bid.reset();
            
            if (forced_bid) {

                // If Player
                if ( (_player_turn == PLAYER_0) && _human_player) {
                    _bid = std::shared_ptr<KaiserBid>(new KaiserBid(_minimum_bid,false));  // Forced to make minimum bid
                    
                } else {
                    // Do AI bid
                    _bid = _ai[_player_turn].bid(_player_turn, _dealer, _players[_player_turn].cards(), _minimum_bid, forced_bid, _highest_bidder_bid);
                    ASSERT(_bid);
                }

            } else {
            
                // If Player
                if ( (_player_turn == PLAYER_0) && _human_player) {
                
                    // Show
                    show_bidding();
                    
                    // Wait for bid
                    while (!_bid)
                        _co.yield(0.0F);
                    
                    ASSERT(_bid);
                } else {
                
                    // Do AI bid
                    _bid = _ai[_player_turn].bid(_player_turn, _dealer, _players[_player_turn].cards(), _minimum_bid, forced_bid, _highest_bidder_bid);
                    ASSERT(_bid);
                }
                
            }
            
            // Replace existing bid maybe
            if ((!_highest_bidder_bid) ||                                   // No prev bid
                (forced_bid) ||
                ( is_dealer && ( (*_bid) >= (*_highest_bidder_bid))) ||     // Dealer matches
                ( !is_dealer && ( (*_bid) > (*_highest_bidder_bid)))) {     // Player beats
                
                _highest_bidder_bid = _bid;
                _highest_bidder = _player_turn;
            } else {
                // Replace with Pass
                _bid = std::shared_ptr<KaiserBid>(new KaiserBid());
            }
            
            animate_bid (_player_turn, _bid);
        }


        _co.yield(best_delay);

        animate_hide_player_message(PLAYER_0);
        animate_hide_player_message(PLAYER_1);
        animate_hide_player_message(PLAYER_2);
        animate_hide_player_message(PLAYER_3);

        // Switch to winner
        _player_turn = _highest_bidder;
        _first_player_turn = _highest_bidder;
        
        animate_player_turn (_player_turn);
        
        //
        // 5. Trump
        //
        
        // RULE: After a successful bid, the person who won the bid declares trump (unless it
        // was a no-trump bid) and plays any card they choose.
        if (!_highest_bidder_bid->is_no()) {
        
            LOG_MESSAGE << "Trump";
            
            // If Player
            if ( (_player_turn == PLAYER_0) && _human_player) {
            
                // Show
                show_trump(forced_bid);
                
                // Wait for bid
                while (_highest_bidder_bid->trump() == SUIT_UNDEFINED && !_highest_bidder_bid->is_no())
                    _co.yield(0.0F);
                
            } else {
            
                // Do AI bid
//                if (forced_bid)
//                    _ai[_player_turn].force_trump(_player_turn, _players[_player_turn].cards(), _minimum_bid, _highest_bidder_bid);
//                else
                    _ai[_player_turn].trump(_player_turn, _players[_player_turn].cards(), forced_bid, _highest_bidder_bid);
            }
            
            animate_trump (_player_turn, _highest_bidder_bid);
        }
        
        animate_update_score ();

        _co.yield(best_delay);
        
        //
        // 6. Playing cards
        //


        _interactions_component->set_played_cb(make_callback(this, &KaiserGameController::play_card));

        // Do 8 times
        for (DTshort c = 0; c < 8; ++c) {

            // Wait for animations to get done
            while (PropertyAnimator::is_animating())
                _co.yield(0.0F);

            // Clear played cards
            for (DTshort p = 0; p < 4; ++p) {
                _played_cards[p].reset();
                _players[p].clear_limits();
            }

            // Do round
            for (DTshort p = 0; p < 4; ++p) {
                _player_turn = (Player)((_first_player_turn + p) % 4);

                if (_player_turn != PLAYER_0 || !_human_player)
                    _co.yield(best_delay + (MoreMath::random_float() - 0.5F) * best_delay_variation);
                else
                    _co.yield(best_delay * 0.1F);

                animate_player_turn (_player_turn);

                // If Player
                if ( (_player_turn == PLAYER_0) && _human_player ) {
                    
                    // Allow player to interact with card
                    _interactions_component->set_allow_interactions(true);
                    
                    // Wait for bid
                    while (!_played_cards[PLAYER_0])
                        _co.yield(0.0F);

                    LOG_MESSAGE << " Player played: " << _played_cards[PLAYER_0]->value() << "," << _played_cards[PLAYER_0]->suit();
                    
                    // Allow player to interact with card
                    _interactions_component->set_allow_interactions(false);
                    
                } else {
                
                    // AI
                    _played_cards[_player_turn] = _ai[_player_turn].play(_player_turn, _first_player_turn, _players[_player_turn].cards(),_played_cards,_highest_bidder_bid);
                    ASSERT(_played_cards[_player_turn]);
                    ASSERT(_played_cards[_player_turn]->is_trick() == false);
                }
                
                // Remove card from hand
                _played_cards[_player_turn]->set_can_interact(false);
                _players[_player_turn].remove_card(_played_cards[_player_turn]);
                
                // Animate card into position
                auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(_played_cards[_player_turn]),
                                                    &PlaceableObject::transform,
                                                    &PlaceableObject::set_transform);
                
                // Deal sound
                h->append(0.0F, make_latent_call(this,&KaiserGameController::play_sound_deal));

                h->append(played_card_transform(_player_turn), 0.2F, 0.0F, std::make_shared<PropertyAnimatorCard>());

                // RULE: Players must follow suit if able (they cannot "trump in" if they have a
                // card in the suit that was led).
                // So lets only enable certain cards.
                if (_player_turn == _first_player_turn) {
                    for (DTshort p = 0; p < 4; ++p) {
                        if (_player_turn != p)
                            _players[p].limit (_played_cards[_player_turn]->suit());
                    }
                }

            }

            _co.yield(best_delay);


            // Wait for animations to get done
            while (PropertyAnimator::is_animating())
                _co.yield(0.0F);

            // Clear limits
            for (DTshort p = 0; p < 4; ++p) {
                _players[p].clear_limits();
            }

            // Check winner
            Player winner = PLAYER_0;
            
            Suit first_played_suit = _played_cards[_first_player_turn]->suit();
            Suit trump_suit = _highest_bidder_bid->trump();
            
            // Compare to other players
            for (DTshort p = 1; p < 4; ++p) {
                if (is_greater_than(_played_cards[p], _played_cards[winner], trump_suit, first_played_suit, SUIT_UNDEFINED))
                    winner = (Player) p;
            }

            Player trick_winner = winner;
            if (trick_winner == PLAYER_2) trick_winner = PLAYER_0;
            if (trick_winner == PLAYER_3) trick_winner = PLAYER_1;

            _players[trick_winner].add_trick(_played_cards);

            // Clear trick flag from other cards
            for (auto &i : _cards) {
                i->set_trick(false);
            }

            // Wait for animations to get done
            while (PropertyAnimator::is_animating())
                _co.yield(0.0F);

            // Animate trick into position
            for (DTshort p = 0; p < 4; ++p) {
                ASSERT(_played_cards[p]->is_trick() == false);

                _played_cards[p]->set_trick(true);

                auto h = PropertyAnimator::animate( checked_cast<PlaceableObject>(_played_cards[p]),
                                                    &PlaceableObject::transform,
                                                    &PlaceableObject::set_transform);
                
                h->append(trick_card_transform(trick_winner, _players[trick_winner].number_of_tricks()-1), 0.2F, 0.0F, std::make_shared<PropertyAnimatorCard>());
            }

            // Finalize AI
            for (DTshort p = 0; p < 4; ++p) {
                _ai[p].finalize( (Player) p, _first_player_turn,_played_cards,_highest_bidder_bid);
            }

            // Change first player
            _first_player_turn = winner;
            
            
            //
            // 7. Tally round score
            //
            
            _us_score_round = _players[PLAYER_0].score() + _players[PLAYER_2].score();
            _them_score_round = _players[PLAYER_1].score() + _players[PLAYER_3].score();
            
            animate_update_score();
            
            _co.yield(best_delay);


            if (_highest_bidder == PLAYER_0 || _highest_bidder == PLAYER_2) {
                LOG_MESSAGE << "Dealer performance: " << _players[PLAYER_0].score() + _players[PLAYER_2].score() << "," << _highest_bidder_bid->bid();
            } else {
                LOG_MESSAGE << "Dealer performance: " << _players[PLAYER_1].score() + _players[PLAYER_3].score() << "," << _highest_bidder_bid->bid();
            }
        }
        

        //
        // 10. Tally final score
        //
        
        DTshort *_bidder_score_round_ptr = &_us_score_round;
        DTshort *_bidder_score_ptr = &_us_score;
        DTshort *_non_bidder_score_round_ptr = &_them_score_round;
        DTshort *_non_bidder_score_ptr = &_them_score;
        
        if (_highest_bidder == PLAYER_0 || _highest_bidder == PLAYER_2) {
            _bidder_score_round_ptr = &_us_score_round;
            _bidder_score_ptr = &_us_score;
            _non_bidder_score_round_ptr = &_them_score_round;
            _non_bidder_score_ptr = &_them_score;
        } else {
            _bidder_score_round_ptr = &_them_score_round;
            _bidder_score_ptr = &_them_score;
            _non_bidder_score_round_ptr = &_us_score_round;
            _non_bidder_score_ptr = &_us_score;
        }
        
        //
        // Check Kaiser Bid
        //
        
        if (_highest_bidder_bid->is_no() && _highest_bidder_bid->bid() == 12) {
            
            if ( (*_bidder_score_round_ptr) == 12 && (*_non_bidder_score_round_ptr) == 0) {
                animate_win();
            } else {
                animate_lose();
            }
        
            // Never re-enter game, we're done
            Analytics::record_event("Kaiser", "GameState", "Done");
            _co.yield(std::numeric_limits<DTfloat>::infinity());
        
        }

        DTshort end_score = 52;
        if (_option_no_trump_bidout && _highest_bidder_bid->is_no())
            end_score = 62;

        //
        // Non bidder
        //

        if ((*_non_bidder_score_ptr) < (52-_minimum_bid)) {
            (*_non_bidder_score_ptr) += (*_non_bidder_score_round_ptr);
        }
        (*_non_bidder_score_round_ptr) = 0;


        //
        // Bidders
        //
    
        // If the bidding team made at least the amount they bid, they score
        // the number of points they made
        if ( (*_bidder_score_round_ptr) >= _highest_bidder_bid->bid()) {
            
            // twice that amount for a no-trump bid
            if (_highest_bidder_bid->is_no()) {
                (*_bidder_score_round_ptr) *= 2.0F;
            }

            // Increase if won or not in bid-out
            if (((*_bidder_score_ptr) >= (52-_minimum_bid)) && (((*_bidder_score_ptr) + (*_bidder_score_round_ptr)) >= end_score))
                (*_bidder_score_ptr) += (*_bidder_score_round_ptr);
            else if ((*_bidder_score_ptr) < (52-_minimum_bid))
                (*_bidder_score_ptr) += (*_bidder_score_round_ptr);

            (*_bidder_score_round_ptr) = 0;
            
        // If they did not, they lose the amount they bid
        } else {
            (*_bidder_score_round_ptr) = -_highest_bidder_bid->bid();
            
            // twice that amount for a no-trump bid
            if (_highest_bidder_bid->is_no()) {
                (*_bidder_score_round_ptr) *= 2.0F;
            }
            
            (*_bidder_score_ptr) += (*_bidder_score_round_ptr);
            (*_bidder_score_round_ptr) = 0;
            
        }


//        // OPTION: Check for stealing 5 of hearts
//        if (_option_steal_5_is_win) {
//
//            // If in bidout
//            if (*_bidder_score_ptr >= (52-_minimum_bid)) {
//
//                if (_highest_bidder == PLAYER_0 || _highest_bidder == PLAYER_2) {
//
//                    if (_players[PLAYER_1].has_5_hearts() || _players[PLAYER_3].has_5_hearts()) {
//                        animate_lose();
//                        
//                        // Never re-enter game, we're done
//                        Analytics::record_event("Kaiser", "GameState", "Done");
//                        _co.yield(std::numeric_limits<DTfloat>::infinity());
//                    }
//
//                } else {
//
//                    if (_players[PLAYER_0].has_5_hearts() || _players[PLAYER_2].has_5_hearts()) {
//                        animate_win();
//                        
//                        // Never re-enter game, we're done
//                        Analytics::record_event("Kaiser", "GameState", "Done");
//                        _co.yield(std::numeric_limits<DTfloat>::infinity());
//                    }
//
//                }
//                
//
//            }
//
//        }

        

        animate_update_score();
        _co.yield(1.0F);

        _highest_bidder_bid.reset();
        animate_update_score();

        //
        // 11. Check end game
        //

        // Check for end game
        if (_us_score >= end_score && _them_score >= end_score) {
            animate_tie();
            
            // Never re-enter game, we're done
            Analytics::record_event("Kaiser", "GameState", "Done");
            _co.yield(std::numeric_limits<DTfloat>::infinity());
        
        } else if (_us_score >= end_score) {
            animate_win();
            
            // Never re-enter game, we're done
            Analytics::record_event("Kaiser", "GameState", "Done");
            _co.yield(std::numeric_limits<DTfloat>::infinity());
        } else if (_them_score >= end_score) {
            animate_lose();
            
            // Never re-enter game, we're done
            Analytics::record_event("Kaiser", "GameState", "Done");
            _co.yield(std::numeric_limits<DTfloat>::infinity());
        }

        // OPTIONAL: Game over at -52
        if (_option_game_over_52) {
            if (_us_score <= -52) {
                animate_lose();
                
                // Never re-enter game, we're done
                Analytics::record_event("Kaiser", "GameState", "Done");
                _co.yield(std::numeric_limits<DTfloat>::infinity());
            } else if (_them_score <= -52) {
                animate_win();
                
                // Never re-enter game, we're done
                Analytics::record_event("Kaiser", "GameState", "Done");
                _co.yield(std::numeric_limits<DTfloat>::infinity());
            }
        }

        LOG_MESSAGE << "Running";
    
        // Done!
        _co.yield(2.0F);


    }

}

//==============================================================================
//==============================================================================

void KaiserGameController::tick (const DTfloat dt)
{
    _co.resume();
}

//==============================================================================
//==============================================================================

void KaiserGameController::play_sound_shuffle (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{shuffling1.wav}"), world());
}

void KaiserGameController::play_sound_deal (void)
{
    if (_pause_sound_on) {
        auto source = System::audio_renderer()->play_quick(FilePath("{dealing.wav}"), world());
        
        source->set_gain(0.3F);
        source->set_pitch(1.0F + (MoreMath::random_float() - 0.5F) * 0.4F);
    }
}

void KaiserGameController::play_sound_click (void)
{
    if (_pause_sound_on)
        System::audio_renderer()->play_quick(FilePath("{click.wav}"), world());
}

void KaiserGameController::play_sound_bubble (void)
{
    if (_pause_sound_on) {
        auto source = System::audio_renderer()->play_quick(FilePath("{bubble.wav}"), world());
    
        source->set_pitch(1.0F + (MoreMath::random_float() - 0.5F) * 0.4F);
    }
}

//==============================================================================
//==============================================================================

std::shared_ptr<GUIObject> KaiserGameController::make_key(const std::string &key, DTfloat width, DTfloat height)
{
    DTfloat screen_width = System::renderer()->screen_width();
    DTfloat screen_height = System::renderer()->screen_height();

    DTboolean phone_mode = screen_height > screen_width;
    if (!phone_mode) {
        DTfloat scale = screen_width / 1024.0F;
        screen_width /= scale;
        screen_height /= scale;
    }

    std::shared_ptr<GUIObject> widget = GUIObject::create();
    world()->add_node_unique_name(widget);
    _gui_controller->add_child(widget);
    
    widget->set_label("{FMT_BUTTON}" + key);
    widget->set_width(width);
    widget->set_height(height);
    widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
    
    // Interaction 
    std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
    widget->add_component(widget_interaction);
    widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&type::click_key, key));

    // Drawing
    std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
    widget->add_component(widget_drawing);
    
    widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
    widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
    widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
    widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    widget_drawing->set_corner_height(0.025F);
    widget_drawing->set_corner_width(0.025F);

    return widget;
}

std::shared_ptr<GUIObject> KaiserGameController::make_key_space(DTfloat width, DTfloat height)
{
    DTfloat screen_width = System::renderer()->screen_width();
    DTfloat screen_height = System::renderer()->screen_height();
    
    DTboolean phone_mode = screen_height > screen_width;
    if (!phone_mode) {
        DTfloat scale = screen_width / 1024.0F;
        screen_width /= scale;
    }

    // Make it bigger
    std::shared_ptr<GUIObject> space = make_key(" ", 350.0F / screen_width, height);
    return space;
}

//==============================================================================
//==============================================================================

void KaiserGameController::add_to_world(World *world)
{
    GameController::add_to_world(world);
    
    world->register_for_tick(this, make_callback(this, &type::tick));

#ifndef DT3_EDITOR
    
    // Add other cards
    for (DTuint i = 0; i < 52; ++i) {
    
        DTshort c = i % 13;
        DTshort s = i / 13;
        
        if (    (c < 6) &&
                (s != 0 || c != 5) &&   // 7 club
                (s != 1 || c != 5) &&   // 7 diamond
                (s != 2 || c != 3) &&   // 5 hearts
                (s != 3 || c != 1) )    // 3 spade
            continue;

        std::string suit;
        
        switch (s) {
            case SUIT_CLUB:     suit = "club";  break;
            case SUIT_DIAMOND:  suit = "diamond";  break;
            case SUIT_HEART:    suit = "heart";  break;
            case SUIT_SPADE:    suit = "spade";  break;
        }

        std::string card;
        
        switch (c) {
            case CARD_ACE: card = "1"; break;
            case CARD_KING: card = "king"; break;
            case CARD_QUEEN: card = "queen"; break;
            case CARD_JACK: card = "jack"; break;
            case CARD_10: card = "10"; break;
            case CARD_9: card = "9"; break;
            case CARD_8: card = "8"; break;
            case CARD_7: card = "7"; break;
            case CARD_6: card = "6"; break;
            case CARD_5: card = "5"; break;
            case CARD_4: card = "4"; break;
            case CARD_3: card = "3"; break;
            case CARD_2: card = "2"; break;
        }
        
        // Build the card
        std::shared_ptr<KaiserObjectCard> cc = KaiserObjectCard::create();
        
        cc->set_orientation(flip_over(Matrix3::identity()));
        
        cc->load(FilePath("{" + card + "_" + suit + ".mat}"), card + "_" + suit, (Suit) s, (Card) c);
        world->add_node_unique_name(cc);
        
        _cards.push_back(cc);
        
    }
    
    
    DTboolean full_version = MoreStrings::cast_from_string<DTboolean>(Globals::global("APP_FULL_VERSION"));

    DTfloat screen_width = System::renderer()->screen_width();
    DTfloat screen_height = System::renderer()->screen_height();

    DTboolean phone_mode = screen_height > screen_width;
    if (!phone_mode) {
        DTfloat scale = screen_width / 1024.0F;
        screen_width /= scale;
        screen_height /= scale;
    }

    _gui_layout_master = GUIGridLayout::create();
    _gui_layout_master->set_rows_and_columns(3, 3);
    if (phone_mode)
        _gui_layout_master->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 150.0F/screen_width));
    else
        _gui_layout_master->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_width));
    _gui_layout_master->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_master->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_width));
    _gui_layout_master->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_height));
    _gui_layout_master->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_master->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_height));
    
    
    // Build GUI
    _gui_controller = GUIController::create();
    _gui_controller->set_use_stencil(false);
    world->add_node_unique_name(_gui_controller);
    
    
    // Interactions
    {
        _interactions = GUIObject::create();
        world->add_node_unique_name(_interactions);
        _gui_controller->add_child(_interactions);
        
        _interactions->set_translation(Vector3(0.5F,0.5F,0.0F));
        _interactions->set_width(1.0F);
        _interactions->set_height(1.0F);

        // Interaction 
        _interactions_component = KaiserInteraction::create();
        _interactions->add_component(_interactions_component);
    }
    
    
    //  _____                    _                         _
    // / ____|                  | |                       | |
    //| (___   ___ ___  _ __ ___| |__   ___   __ _ _ __ __| |
    // \___ \ / __/ _ \| '__/ _ \ '_ \ / _ \ / _` | '__/ _` |
    // ____) | (_| (_) | | |  __/ |_) | (_) | (_| | | | (_| |
    //|_____/ \___\___/|_|  \___|_.__/ \___/ \__,_|_|  \__,_|
    //                                                       
    
    
    _gui_layout_scoreboard = GUIGridLayout::create();
    
    if (phone_mode) {
        _gui_layout_scoreboard->set_rows_and_columns(6, 6);
        _gui_layout_scoreboard->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_scoreboard->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_height));
        _gui_layout_scoreboard->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_height));
        _gui_layout_scoreboard->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_height));
        _gui_layout_scoreboard->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_height));

        if (full_version) {
            _gui_layout_scoreboard->set_row_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_height));
        } else {
            _gui_layout_scoreboard->set_row_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_height));
        }
        
        _gui_layout_scoreboard->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 300.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_scoreboard->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 175.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 5.0F/screen_width));

    } else {
        _gui_layout_scoreboard->set_rows_and_columns(4, 7);
        _gui_layout_scoreboard->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_scoreboard->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_height));
        _gui_layout_scoreboard->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_height));
        
        if (full_version) {
            _gui_layout_scoreboard->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_height));
        } else {
            _gui_layout_scoreboard->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));
        }
        
        _gui_layout_scoreboard->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 300.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_scoreboard->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 110.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 110.0F/screen_width));
        _gui_layout_scoreboard->set_column_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 32.0F/screen_width));
    }

    // Score
    if (!phone_mode) {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_layout_scoreboard->add_item(2,2,widget);
        
        widget->set_label("{TXT_SCORE}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_center_vertically(true);
    }



    // Us
    {
        _us_widget_title = GUIObject::create();
        world->add_node_unique_name(_us_widget_title);
        _gui_controller->add_child(_us_widget_title);
        
        if (phone_mode)     _gui_layout_scoreboard->add_item(4,4,_us_widget_title);
        else                _gui_layout_scoreboard->add_item(2,4,_us_widget_title);
        
        _us_widget_title->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _us_widget_title->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_center_vertically(true);
    }

    // Them
    {
        _them_widget_title = GUIObject::create();
        world->add_node_unique_name(_them_widget_title);
        _gui_controller->add_child(_them_widget_title);

        if (phone_mode)     _gui_layout_scoreboard->add_item(2,4,_them_widget_title);
        else                _gui_layout_scoreboard->add_item(2,5,_them_widget_title);
        
        _them_widget_title->set_label("{TXT_THEM}");
        _them_widget_title->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _them_widget_title->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_center_vertically(true);
    }

    // Us Score
    {
        _us_widget = GUIObject::create();
        world->add_node_unique_name(_us_widget);
        _gui_controller->add_child(_us_widget);
        
        if (phone_mode)     _gui_layout_scoreboard->add_item(3,4,_us_widget);
        else                _gui_layout_scoreboard->add_item(1,4,_us_widget);
        
        _us_widget->set_label("");
        _us_widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _us_widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_center_vertically(true);
    }

    // Them Score
    {
        _them_widget = GUIObject::create();
        world->add_node_unique_name(_them_widget);
        _gui_controller->add_child(_them_widget);

        if (phone_mode)     _gui_layout_scoreboard->add_item(1,4,_them_widget);
        else                _gui_layout_scoreboard->add_item(1,5,_them_widget);
        
        _them_widget->set_label("");
        _them_widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _them_widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_center_vertically(true);
    }
    

    
    // _______
    //|__   __|              
    //   | |_   _ _ __ _ __  
    //   | | | | | '__| '_ \ 
    //   | | |_| | |  | | | |
    //   |_|\__,_|_|  |_| |_|
    #pragma mark Turn

    _gui_layout_turn = GUIGridLayout::create();
    _gui_layout_turn->set_rows_and_columns(5, 5);
    _gui_layout_turn->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 40.0F/screen_height));
    _gui_layout_turn->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_turn->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 40.0F/screen_height));
    _gui_layout_turn->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_turn->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 40.0F/screen_height));
    _gui_layout_turn->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 40.0F/screen_width));
    _gui_layout_turn->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_turn->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 40.0F/screen_width));
    _gui_layout_turn->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_turn->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 40.0F/screen_width));
    
    // Player 1
    {
        _turn_player_0_widget = GUIObject::create();
        world->add_node_unique_name(_turn_player_0_widget);
        _gui_controller->add_child(_turn_player_0_widget);
        
        _gui_layout_turn->add_item(0,2,_turn_player_0_widget);
        
        _turn_player_0_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _turn_player_0_widget->set_no_focus(true);
//        _turn_player_0_widget->set_scale(Vector3(0.0F,0.0F,0.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _turn_player_0_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_arrow_up.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Player 2
    {
        _turn_player_1_widget = GUIObject::create();
        world->add_node_unique_name(_turn_player_1_widget);
        _gui_controller->add_child(_turn_player_1_widget);
        
        _gui_layout_turn->add_item(2,0,_turn_player_1_widget);
        
        _turn_player_1_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _turn_player_1_widget->set_no_focus(true);
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _turn_player_1_widget->add_component(widget_drawing);
//        _turn_player_1_widget->set_scale(Vector3(0.0F,0.0F,0.0F));

        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_arrow_right.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Player 3
    {
        _turn_player_2_widget = GUIObject::create();
        world->add_node_unique_name(_turn_player_2_widget);
        _gui_controller->add_child(_turn_player_2_widget);
        
        _gui_layout_turn->add_item(4,2,_turn_player_2_widget);
        
        _turn_player_2_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _turn_player_2_widget->set_no_focus(true);
//        _turn_player_2_widget->set_scale(Vector3(0.0F,0.0F,0.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _turn_player_2_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_arrow_down.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Player 4
    {
        _turn_player_3_widget = GUIObject::create();
        world->add_node_unique_name(_turn_player_3_widget);
        _gui_controller->add_child(_turn_player_3_widget);
        
        _gui_layout_turn->add_item(2,4,_turn_player_3_widget);
        
        _turn_player_3_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _turn_player_3_widget->set_no_focus(true);
//        _turn_player_3_widget->set_scale(Vector3(0.0F,0.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _turn_player_3_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_arrow_left.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }


    // Dealer 1
    {
        _dealer_player_0_widget = GUIObject::create();
        world->add_node_unique_name(_dealer_player_0_widget);
        _gui_controller->add_child(_dealer_player_0_widget);
        
        _gui_layout_turn->add_item(0,2,_dealer_player_0_widget);
        
        _dealer_player_0_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _dealer_player_0_widget->set_no_focus(true);
//        _dealer_player_0_widget->set_scale(Vector3(0.0F,0.0F,0.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _dealer_player_0_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_dealer.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Dealer 2
    {
        _dealer_player_1_widget = GUIObject::create();
        world->add_node_unique_name(_dealer_player_1_widget);
        _gui_controller->add_child(_dealer_player_1_widget);
        
        _gui_layout_turn->add_item(2,0,_dealer_player_1_widget);
        
        _dealer_player_1_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _dealer_player_1_widget->set_no_focus(true);
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _dealer_player_1_widget->add_component(widget_drawing);

        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_dealer.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Dealer 3
    {
        _dealer_player_2_widget = GUIObject::create();
        world->add_node_unique_name(_dealer_player_2_widget);
        _gui_controller->add_child(_dealer_player_2_widget);
        
        _gui_layout_turn->add_item(4,2,_dealer_player_2_widget);
        
        _dealer_player_2_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _dealer_player_2_widget->set_no_focus(true);
//        _dealer_player_2_widget->set_scale(Vector3(0.0F,0.0F,0.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _dealer_player_2_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_dealer.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Dealer 4
    {
        _dealer_player_3_widget = GUIObject::create();
        world->add_node_unique_name(_dealer_player_3_widget);
        _gui_controller->add_child(_dealer_player_3_widget);
        
        _gui_layout_turn->add_item(2,4,_dealer_player_3_widget);
        
        _dealer_player_3_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _dealer_player_3_widget->set_no_focus(true);
//        _dealer_player_3_widget->set_scale(Vector3(0.0F,0.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _dealer_player_3_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_dealer.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    
    //
    // __  __                                     
    //|  \/  |                                    
    //| \  / | ___  ___ ___  __ _  __ _  ___  ___ 
    //| |\/| |/ _ \/ __/ __|/ _` |/ _` |/ _ \/ __|
    //| |  | |  __/\__ \__ \ (_| | (_| |  __/\__ \
    //|_|  |_|\___||___/___/\__,_|\__, |\___||___/
    //                             __/ |          
    //                            |___/
    #pragma mark Messages

    _gui_layout_messages = GUIGridLayout::create();
    _gui_layout_messages->set_rows_and_columns(7, 7);
    _gui_layout_messages->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.2F));
    _gui_layout_messages->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_height));
    if (phone_mode)
        _gui_layout_messages->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 5.0F));
    else
        _gui_layout_messages->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_messages->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_height));
    _gui_layout_messages->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_messages->set_row_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_height));
    _gui_layout_messages->set_row_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.2F));

    _gui_layout_messages->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 2.0F));
    _gui_layout_messages->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_width));
    _gui_layout_messages->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_messages->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_width));
    _gui_layout_messages->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_messages->set_column_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_width));
    _gui_layout_messages->set_column_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 2.0F));

    // Bottom
    {
        _message_player_0_widget = GUIObject::create();
        world->add_node_unique_name(_message_player_0_widget);
        _gui_controller->add_child(_message_player_0_widget);
        
        _gui_layout_messages->add_item(1,3,_message_player_0_widget);
        
        _message_player_0_widget->set_label("{BID7}");
        _message_player_0_widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        _message_player_0_widget->set_scale(Vector3(0.0F,0.0F,0.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _message_player_0_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_speech_down.mat}")));
        widget_drawing->set_pressed_material(MaterialResource::import_resource(FilePath("{ui_speech_down.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.03F);
        widget_drawing->set_corner_width(0.03F);
        widget_drawing->set_draw_style(ComponentGUIDrawButton::DRAW_STYLE_STRETCH_CENTER_2X2);
    }

    // Left
    {
        _message_player_1_widget = GUIObject::create();
        world->add_node_unique_name(_message_player_1_widget);
        _gui_controller->add_child(_message_player_1_widget);
        
        _gui_layout_messages->add_item(3,1,_message_player_1_widget);
        
        _message_player_1_widget->set_label("{BID7}");
        _message_player_1_widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        _message_player_1_widget->set_scale(Vector3(0.0F,0.0F,0.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _message_player_1_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_speech_left.mat}")));
        widget_drawing->set_pressed_material(MaterialResource::import_resource(FilePath("{ui_speech_left.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.03F);
        widget_drawing->set_corner_width(0.03F);
        widget_drawing->set_draw_style(ComponentGUIDrawButton::DRAW_STYLE_STRETCH_CENTER_2X2);
    }

    // Top
    {
        _message_player_2_widget = GUIObject::create();
        world->add_node_unique_name(_message_player_2_widget);
        _gui_controller->add_child(_message_player_2_widget);
        
        _gui_layout_messages->add_item(5,3,_message_player_2_widget);
        
        _message_player_2_widget->set_label("{BID7}");
        _message_player_2_widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        _message_player_2_widget->set_scale(Vector3(0.0F,0.0F,0.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _message_player_2_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_speech_up.mat}")));
        widget_drawing->set_pressed_material(MaterialResource::import_resource(FilePath("{ui_speech_up.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.03F);
        widget_drawing->set_corner_width(0.03F);
        widget_drawing->set_draw_style(ComponentGUIDrawButton::DRAW_STYLE_STRETCH_CENTER_2X2);
    }

    // Right
    {
        _message_player_3_widget = GUIObject::create();
        world->add_node_unique_name(_message_player_3_widget);
        _gui_controller->add_child(_message_player_3_widget);
        
        _gui_layout_messages->add_item(3,5,_message_player_3_widget);
        
        _message_player_3_widget->set_label("{BID7}");
        _message_player_3_widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        _message_player_3_widget->set_scale(Vector3(0.0F,0.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _message_player_3_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_speech_right.mat}")));
        widget_drawing->set_pressed_material(MaterialResource::import_resource(FilePath("{ui_speech_right.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.03F);
        widget_drawing->set_corner_width(0.03F);
        widget_drawing->set_draw_style(ComponentGUIDrawButton::DRAW_STYLE_STRETCH_CENTER_2X2);
    }

    // _____             _ _
    //|  __ \           | (_)            
    //| |  | | ___  __ _| |_ _ __   __ _ 
    //| |  | |/ _ \/ _` | | | '_ \ / _` |
    //| |__| |  __/ (_| | | | | | | (_| |
    //|_____/ \___|\__,_|_|_|_| |_|\__, |
    //                              __/ |
    //                             |___/
    #pragma mark Dealing
    
    

    // __  __ _         ____  _     _
    //|  \/  (_)       |  _ \(_)   | |
    //| \  / |_ _ __   | |_) |_  __| |
    //| |\/| | | '_ \  |  _ <| |/ _` |
    //| |  | | | | | | | |_) | | (_| |
    //|_|  |_|_|_| |_| |____/|_|\__,_|
    //

    DTfloat button_size = phone_mode ? 100.0F : 64.0F;

    _gui_layout_options_min_bidding = GUIGridLayout::create();
    _gui_layout_options_min_bidding->set_rows_and_columns(11, 1);
    _gui_layout_options_min_bidding->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_options_min_bidding->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_options_min_bidding->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_height));
    _gui_layout_options_min_bidding->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_options_min_bidding->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_options_min_bidding->set_row_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_options_min_bidding->set_row_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_height));
    _gui_layout_options_min_bidding->set_row_policy(7, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_options_min_bidding->set_row_policy(8, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_options_min_bidding->set_row_policy(9, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_options_min_bidding->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_master->add_item(1, 1, _gui_layout_options_min_bidding);
    
    std::shared_ptr<GUIGridLayout> gui_layout_options_min_bidding_buttons_0 = GUIGridLayout::create();
    gui_layout_options_min_bidding_buttons_0->set_rows_and_columns(1, 6);
    gui_layout_options_min_bidding_buttons_0->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_0->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_0->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_options_min_bidding_buttons_0->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_options_min_bidding_buttons_0->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_options_min_bidding_buttons_0->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_options_min_bidding_buttons_0->set_column_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_options_min_bidding->add_item(7, 0, gui_layout_options_min_bidding_buttons_0);


//    std::shared_ptr<GUIGridLayout> gui_layout_options_min_bidding_buttons_2 = GUIGridLayout::create();
//    gui_layout_options_min_bidding_buttons_2->set_rows_and_columns(1, 3);
//    gui_layout_options_min_bidding_buttons_2->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
//    gui_layout_options_min_bidding_buttons_2->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
//    gui_layout_options_min_bidding_buttons_2->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
//    gui_layout_options_min_bidding_buttons_2->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 1.5F*button_size/screen_width));
//    _gui_layout_options_min_bidding->add_item(3, 0, gui_layout_options_min_bidding_buttons_2);

    std::shared_ptr<GUIGridLayout> gui_layout_options_min_bidding_buttons_3 = GUIGridLayout::create();
    gui_layout_options_min_bidding_buttons_3->set_rows_and_columns(1, 3);
    gui_layout_options_min_bidding_buttons_3->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_3->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_3->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
    gui_layout_options_min_bidding_buttons_3->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 1.5F*button_size/screen_width));
    _gui_layout_options_min_bidding->add_item(3, 0, gui_layout_options_min_bidding_buttons_3);

    std::shared_ptr<GUIGridLayout> gui_layout_options_min_bidding_buttons_4 = GUIGridLayout::create();
    gui_layout_options_min_bidding_buttons_4->set_rows_and_columns(1, 3);
    gui_layout_options_min_bidding_buttons_4->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_4->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_4->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
    gui_layout_options_min_bidding_buttons_4->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 1.5F*button_size/screen_width));
    _gui_layout_options_min_bidding->add_item(4, 0, gui_layout_options_min_bidding_buttons_4);

    std::shared_ptr<GUIGridLayout> gui_layout_options_min_bidding_buttons_5 = GUIGridLayout::create();
    gui_layout_options_min_bidding_buttons_5->set_rows_and_columns(1, 3);
    gui_layout_options_min_bidding_buttons_5->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_5->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_5->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
    gui_layout_options_min_bidding_buttons_5->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 1.5F*button_size/screen_width));
    _gui_layout_options_min_bidding->add_item(5, 0, gui_layout_options_min_bidding_buttons_5);


    std::shared_ptr<GUIGridLayout> gui_layout_options_min_bidding_buttons_1 = GUIGridLayout::create();
    gui_layout_options_min_bidding_buttons_1->set_rows_and_columns(1, 3);
    gui_layout_options_min_bidding_buttons_1->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_1->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_options_min_bidding_buttons_1->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
    gui_layout_options_min_bidding_buttons_1->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_options_min_bidding->add_item(1, 0, gui_layout_options_min_bidding_buttons_1);
    
    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_layout_options_min_bidding->set_border_item(widget, 0.05F, 0.05F);
        
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);

    }

    // Title
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_layout_options_min_bidding->add_item(8,0,widget);
        
        widget->set_label("{TXT_CHOOSE_MIN_BID}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        
    }

    
    // Min Bid 5
    {
        _options_min_bid_5_widget = GUIObject::create();
        world->add_node_unique_name(_options_min_bid_5_widget);
        _gui_controller->add_child(_options_min_bid_5_widget);
        
        gui_layout_options_min_bidding_buttons_0->add_item(0,1,_options_min_bid_5_widget);
        
        _options_min_bid_5_widget->set_label("{BID5}");
        _options_min_bid_5_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _options_min_bid_5_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_min_bid, (DTshort) 5));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _options_min_bid_5_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Min Bid 6
    {
        _options_min_bid_6_widget = GUIObject::create();
        world->add_node_unique_name(_options_min_bid_6_widget);
        _gui_controller->add_child(_options_min_bid_6_widget);
        
        gui_layout_options_min_bidding_buttons_0->add_item(0,2,_options_min_bid_6_widget);
        
        _options_min_bid_6_widget->set_label("{BID6}");
        _options_min_bid_6_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _options_min_bid_6_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_min_bid, (DTshort) 6));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _options_min_bid_6_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Min Bid 7
    {
        _options_min_bid_7_widget = GUIObject::create();
        world->add_node_unique_name(_options_min_bid_7_widget);
        _gui_controller->add_child(_options_min_bid_7_widget);
        
        gui_layout_options_min_bidding_buttons_0->add_item(0,3,_options_min_bid_7_widget);
        
        _options_min_bid_7_widget->set_label("{BID7}");
        _options_min_bid_7_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _options_min_bid_7_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_min_bid, (DTshort) 7));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _options_min_bid_7_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Min Bid 8
    {
        _options_min_bid_8_widget = GUIObject::create();
        world->add_node_unique_name(_options_min_bid_8_widget);
        _gui_controller->add_child(_options_min_bid_8_widget);
        
        gui_layout_options_min_bidding_buttons_0->add_item(0,4,_options_min_bid_8_widget);
        
        _options_min_bid_8_widget->set_label("{BID8}");
        _options_min_bid_8_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _options_min_bid_8_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_min_bid, (DTshort) 8));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _options_min_bid_8_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }


    // Title No trump bidout
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        gui_layout_options_min_bidding_buttons_5->add_item(0,0,widget);
        
        widget->set_label("{TXT_NO_TRUMP_BIDOUT}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_center_vertically(true);
    }




    // Title No trump bidout
    {
        _options_no_trump_bidout_widget = GUIObject::create();
        world->add_node_unique_name(_options_no_trump_bidout_widget);
        _gui_controller->add_child(_options_no_trump_bidout_widget);
        
        gui_layout_options_min_bidding_buttons_5->add_item(0,2,_options_no_trump_bidout_widget);
        
        _options_no_trump_bidout_widget->set_label("{BID8}");
        _options_no_trump_bidout_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _options_no_trump_bidout_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_no_trump_bidout));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _options_no_trump_bidout_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }


    // Title Pass Cards
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        gui_layout_options_min_bidding_buttons_4->add_item(0,0,widget);
        
        widget->set_label("{TXT_PASS_CARDS}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_center_vertically(true);
    }

    // Title Pass Cards
    {
        _options_pass_cards_widget = GUIObject::create();
        world->add_node_unique_name(_options_pass_cards_widget);
        _gui_controller->add_child(_options_pass_cards_widget);
        
        gui_layout_options_min_bidding_buttons_4->add_item(0,2,_options_pass_cards_widget);
        
        _options_pass_cards_widget->set_label("{BID8}");
        _options_pass_cards_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _options_pass_cards_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_pass_cards));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _options_pass_cards_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }

    // Title Game over at -52
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        gui_layout_options_min_bidding_buttons_3->add_item(0,0,widget);
        
        widget->set_label("{TXT_GAME_OVER_52}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_center_vertically(true);
    }

    // Title Game over at -52
    {
        _options_game_over_52_widget = GUIObject::create();
        world->add_node_unique_name(_options_game_over_52_widget);
        _gui_controller->add_child(_options_game_over_52_widget);
        
        gui_layout_options_min_bidding_buttons_3->add_item(0,2,_options_game_over_52_widget);
        
        _options_game_over_52_widget->set_label("{BID8}");
        _options_game_over_52_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _options_game_over_52_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_game_over_52));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _options_game_over_52_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }

//    // Title steal 5 is win
//    {
//        std::shared_ptr<GUIObject> widget = GUIObject::create();
//        world->add_node_unique_name(widget);
//        _gui_controller->add_child(widget);
//        
//        gui_layout_options_min_bidding_buttons_2->add_item(0,0,widget);
//        
//        widget->set_label("{TXT_STEAL_5_IS_WIN}");
//        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
//        
//        // Drawing
//        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
//        widget->add_component(widget_drawing);
//        
//        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
//        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
//        widget_drawing->set_center_vertically(true);
//    }
//
//    // Title steal 5 is win
//    {
//        _options_steal_5_to_win_widget = GUIObject::create();
//        world->add_node_unique_name(_options_steal_5_to_win_widget);
//        _gui_controller->add_child(_options_steal_5_to_win_widget);
//        
//        gui_layout_options_min_bidding_buttons_2->add_item(0,2,_options_steal_5_to_win_widget);
//        
//        _options_steal_5_to_win_widget->set_label("{BID8}");
//        _options_steal_5_to_win_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
//        
//        // Interaction 
//        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
//        _options_steal_5_to_win_widget->add_component(widget_interaction);
//        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_steal_5_is_win));
//
//        // Drawing
//        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
//        _options_steal_5_to_win_widget->add_component(widget_drawing);
//        
//        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
//        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
//        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
//        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
//        widget_drawing->set_corner_height(0.025F);
//        widget_drawing->set_corner_width(0.025F);
//    }


    // Set Minimum Bid
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        gui_layout_options_min_bidding_buttons_1->add_item(0,1,widget);
        
        widget->set_label("{SET}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::accept_options_min_bid));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }



        
    // ____  _     _     _ _             
    //|  _ \(_)   | |   | (_)            
    //| |_) |_  __| | __| |_ _ __   __ _ 
    //|  _ <| |/ _` |/ _` | | '_ \ / _` |
    //| |_) | | (_| | (_| | | | | | (_| |
    //|____/|_|\__,_|\__,_|_|_| |_|\__, |
    //                              __/ |
    //                             |___/
    #pragma mark Bidding

    std::shared_ptr<GUIGridLayout> gui_layout_bidding_buttons_0 = GUIGridLayout::create();
    std::shared_ptr<GUIGridLayout> gui_layout_bidding_buttons_0b = GUIGridLayout::create();
    std::shared_ptr<GUIGridLayout> gui_layout_bidding_buttons_1 = GUIGridLayout::create();

    if (phone_mode) {
    
        _gui_layout_bidding = GUIGridLayout::create();
        _gui_layout_bidding->set_rows_and_columns(7, 1);
        _gui_layout_bidding->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_bidding->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
        _gui_layout_bidding->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
        _gui_layout_bidding->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
        _gui_layout_bidding->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
        _gui_layout_bidding->set_row_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
        _gui_layout_bidding->set_row_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_bidding->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_master->add_item(1, 1, _gui_layout_bidding);
    
        gui_layout_bidding_buttons_0->set_rows_and_columns(1, 7);
        gui_layout_bidding_buttons_0->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_0->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_0->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_bidding->add_item(4, 0, gui_layout_bidding_buttons_0);

        gui_layout_bidding_buttons_0b->set_rows_and_columns(1, 7);
        gui_layout_bidding_buttons_0b->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_0b->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_0b->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0b->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0b->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0b->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0b->set_column_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0b->set_column_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_bidding->add_item(3, 0, gui_layout_bidding_buttons_0b);

        gui_layout_bidding_buttons_1->set_rows_and_columns(1, 5);
        gui_layout_bidding_buttons_1->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_1->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_1->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
        gui_layout_bidding_buttons_1->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_1->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
        gui_layout_bidding_buttons_1->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_bidding->add_item(1, 0, gui_layout_bidding_buttons_1);

    } else {

        _gui_layout_bidding = GUIGridLayout::create();
        _gui_layout_bidding->set_rows_and_columns(6, 1);
        _gui_layout_bidding->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 0.5F));
        _gui_layout_bidding->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
        _gui_layout_bidding->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
        _gui_layout_bidding->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
        _gui_layout_bidding->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
        _gui_layout_bidding->set_row_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_bidding->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_master->add_item(1, 1, _gui_layout_bidding);
    
        gui_layout_bidding_buttons_0->set_rows_and_columns(1, 12);
        gui_layout_bidding_buttons_0->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_0->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_0->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(7, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(8, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(9, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(10, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
        gui_layout_bidding_buttons_0->set_column_policy(11, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_bidding->add_item(3, 0, gui_layout_bidding_buttons_0);

        gui_layout_bidding_buttons_1->set_rows_and_columns(1, 5);
        gui_layout_bidding_buttons_1->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_1->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        gui_layout_bidding_buttons_1->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
        gui_layout_bidding_buttons_1->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
        gui_layout_bidding_buttons_1->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
        gui_layout_bidding_buttons_1->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _gui_layout_bidding->add_item(1, 0, gui_layout_bidding_buttons_1);

    }


    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_layout_bidding->set_border_item(widget, 0.05F, 0.05F);
        
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);

    }

    // Title
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);

        if (phone_mode) _gui_layout_bidding->add_item(5,0,widget);
        else            _gui_layout_bidding->add_item(4,0,widget);
        
        widget->set_label("{TXT_PLACE_BID}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        
    }


    // Bids are pass, 5, 6, 7, 8, 9, 10, 11, 12 and No is a toggle
    
    // Bid 5
    {
        _bid_5_widget = GUIObject::create();
        world->add_node_unique_name(_bid_5_widget);
        _gui_controller->add_child(_bid_5_widget);
        
        gui_layout_bidding_buttons_0->add_item(0,1,_bid_5_widget);
        
        _bid_5_widget->set_label("{BID5}");
        _bid_5_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_5_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 5));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_5_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid 6
    {
        _bid_6_widget = GUIObject::create();
        world->add_node_unique_name(_bid_6_widget);
        _gui_controller->add_child(_bid_6_widget);
        
        gui_layout_bidding_buttons_0->add_item(0,2,_bid_6_widget);
        
        _bid_6_widget->set_label("{BID6}");
        _bid_6_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_6_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 6));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_6_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid 7
    {
        _bid_7_widget = GUIObject::create();
        world->add_node_unique_name(_bid_7_widget);
        _gui_controller->add_child(_bid_7_widget);
        
        gui_layout_bidding_buttons_0->add_item(0,3,_bid_7_widget);
        
        _bid_7_widget->set_label("{BID7}");
        _bid_7_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_7_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 7));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_7_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid 8
    {
        _bid_8_widget = GUIObject::create();
        world->add_node_unique_name(_bid_8_widget);
        _gui_controller->add_child(_bid_8_widget);
        
        gui_layout_bidding_buttons_0->add_item(0,4,_bid_8_widget);
        
        _bid_8_widget->set_label("{BID8}");
        _bid_8_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_8_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 8));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_8_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid 9
    {
        _bid_9_widget = GUIObject::create();
        world->add_node_unique_name(_bid_9_widget);
        _gui_controller->add_child(_bid_9_widget);
        
        gui_layout_bidding_buttons_0->add_item(0,5,_bid_9_widget);
        
        _bid_9_widget->set_label("{BID9}");
        _bid_9_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_9_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 9));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_9_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid 10
    {
        _bid_10_widget = GUIObject::create();
        world->add_node_unique_name(_bid_10_widget);
        _gui_controller->add_child(_bid_10_widget);

        if (phone_mode) gui_layout_bidding_buttons_0b->add_item(0,1,_bid_10_widget);
        else            gui_layout_bidding_buttons_0->add_item(0,6,_bid_10_widget);
        
        _bid_10_widget->set_label("{BID10}");
        _bid_10_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_10_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 10));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_10_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid 11
    {
        _bid_11_widget = GUIObject::create();
        world->add_node_unique_name(_bid_11_widget);
        _gui_controller->add_child(_bid_11_widget);
        
        if (phone_mode) gui_layout_bidding_buttons_0b->add_item(0,2,_bid_11_widget);
        else            gui_layout_bidding_buttons_0->add_item(0,7,_bid_11_widget);
        
        _bid_11_widget->set_label("{BID11}");
        _bid_11_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_11_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 11));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_11_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid 12
    {
        _bid_12_widget = GUIObject::create();
        world->add_node_unique_name(_bid_12_widget);
        _gui_controller->add_child(_bid_12_widget);
        
        if (phone_mode) gui_layout_bidding_buttons_0b->add_item(0,3,_bid_12_widget);
        else            gui_layout_bidding_buttons_0->add_item(0,8,_bid_12_widget);
        
        _bid_12_widget->set_label("{BID12}");
        _bid_12_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_12_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 12));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_12_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid No
    {
        _bid_no_widget = GUIObject::create();
        world->add_node_unique_name(_bid_no_widget);
        _gui_controller->add_child(_bid_no_widget);
        
        if (phone_mode) gui_layout_bidding_buttons_0b->add_item(0,5,_bid_no_widget);
        else            gui_layout_bidding_buttons_0->add_item(0,10,_bid_no_widget);
        
        _bid_no_widget->set_label("{NO}");
        _bid_no_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIToggleButton> widget_interaction = ComponentGUIToggleButton::create();
        _bid_no_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_bid, (DTshort) 0));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_no_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }


    // Pass
    {
        _bid_pass_widget = GUIObject::create();
        world->add_node_unique_name(_bid_pass_widget);
        _gui_controller->add_child(_bid_pass_widget);
        
        gui_layout_bidding_buttons_1->add_item(0,1,_bid_pass_widget);
        
        _bid_pass_widget->set_label("{PASS}");
        _bid_pass_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_pass_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::pass_bid));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_pass_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Bid
    {
        _bid_bid_widget = GUIObject::create();
        world->add_node_unique_name(_bid_bid_widget);
        _gui_controller->add_child(_bid_bid_widget);
        
        gui_layout_bidding_buttons_1->add_item(0,3,_bid_bid_widget);
        
        _bid_bid_widget->set_label("{BID}");
        _bid_bid_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _bid_bid_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::accept_bid));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _bid_bid_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // _______
    //|__   __|                        
    //   | |_ __ _   _ _ __ ___  _ __  
    //   | | '__| | | | '_ ` _ \| '_ \ 
    //   | | |  | |_| | | | | | | |_) |
    //   |_|_|   \__,_|_| |_| |_| .__/ 
    //                          | |    
    //                          |_|
    #pragma mark Trump
    
    _gui_layout_declare_trump = GUIGridLayout::create();
    _gui_layout_declare_trump->set_rows_and_columns(6, 1);
    _gui_layout_declare_trump->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_declare_trump->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_declare_trump->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
    _gui_layout_declare_trump->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_declare_trump->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_layout_declare_trump->set_row_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 2.0F));
    _gui_layout_declare_trump->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_master->add_item(1, 1, _gui_layout_declare_trump);
    
    std::shared_ptr<GUIGridLayout> gui_layout_declare_trump_buttons_0 = GUIGridLayout::create();
    gui_layout_declare_trump_buttons_0->set_rows_and_columns(1, 8);
    gui_layout_declare_trump_buttons_0->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_declare_trump_buttons_0->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_declare_trump_buttons_0->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_declare_trump_buttons_0->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_declare_trump_buttons_0->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_declare_trump_buttons_0->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_declare_trump_buttons_0->set_column_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
    gui_layout_declare_trump_buttons_0->set_column_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    gui_layout_declare_trump_buttons_0->set_column_policy(7, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_declare_trump->add_item(3, 0, gui_layout_declare_trump_buttons_0);

    std::shared_ptr<GUIGridLayout> gui_layout_declare_trump_buttons_1 = GUIGridLayout::create();
    gui_layout_declare_trump_buttons_1->set_rows_and_columns(1, 3);
    gui_layout_declare_trump_buttons_1->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_declare_trump_buttons_1->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_declare_trump_buttons_1->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
    gui_layout_declare_trump_buttons_1->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_declare_trump->add_item(1, 0, gui_layout_declare_trump_buttons_1);


    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_layout_declare_trump->set_border_item(widget, 0.05F, 0.05F);
        
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);

    }

    // Title
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_layout_declare_trump->add_item(4,0,widget);
        
        widget->set_label("{TXT_CHOOSE_TRUMP}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Club
    {
        _trump_club_widget = GUIObject::create();
        world->add_node_unique_name(_trump_club_widget);
        _gui_controller->add_child(_trump_club_widget);
        
        gui_layout_declare_trump_buttons_0->add_item(0,1,_trump_club_widget);
        
        _trump_club_widget->set_label("{TXT_CLUB}");
        _trump_club_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _trump_club_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_trump, SUIT_CLUB));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _trump_club_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Diamond
    {
        _trump_diamond_widget = GUIObject::create();
        world->add_node_unique_name(_trump_diamond_widget);
        _gui_controller->add_child(_trump_diamond_widget);
        
        gui_layout_declare_trump_buttons_0->add_item(0,2,_trump_diamond_widget);
        
        _trump_diamond_widget->set_label("{TXT_DIAMOND}");
        _trump_diamond_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _trump_diamond_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_trump, SUIT_DIAMOND));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _trump_diamond_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Heart
    {
        _trump_heart_widget = GUIObject::create();
        world->add_node_unique_name(_trump_heart_widget);
        _gui_controller->add_child(_trump_heart_widget);
        
        gui_layout_declare_trump_buttons_0->add_item(0,3,_trump_heart_widget);
        
        _trump_heart_widget->set_label("{TXT_HEART}");
        _trump_heart_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _trump_heart_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_trump, SUIT_HEART));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _trump_heart_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));

        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Spade
    {
        _trump_spade_widget = GUIObject::create();
        world->add_node_unique_name(_trump_spade_widget);
        _gui_controller->add_child(_trump_spade_widget);
        
        gui_layout_declare_trump_buttons_0->add_item(0,4,_trump_spade_widget);
        
        _trump_spade_widget->set_label("{TXT_SPADE}");
        _trump_spade_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _trump_spade_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_trump, SUIT_SPADE));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _trump_spade_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // No
    {
        _trump_no_widget = GUIObject::create();
        world->add_node_unique_name(_trump_no_widget);
        _gui_controller->add_child(_trump_no_widget);
        
        gui_layout_declare_trump_buttons_0->add_item(0,6,_trump_no_widget);
        
        _trump_no_widget->set_label("{NO}");
        _trump_no_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _trump_no_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_trump, SUIT_NO));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _trump_no_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Go
    {
        _trump_go_widget = GUIObject::create();
        world->add_node_unique_name(_trump_go_widget);
        _gui_controller->add_child(_trump_go_widget);
        
        gui_layout_declare_trump_buttons_1->add_item(0,1,_trump_go_widget);
        
        _trump_go_widget->set_label("{GO}");
        _trump_go_widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _trump_go_widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::accept_trump));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _trump_go_widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }
    

    // ______           _    ____   __    _____
    //|  ____|         | |  / __ \ / _|  / ____|                     
    //| |__   _ __   __| | | |  | | |_  | |  __  __ _ _ __ ___   ___ 
    //|  __| | '_ \ / _` | | |  | |  _| | | |_ |/ _` | '_ ` _ \ / _ \
    //| |____| | | | (_| | | |__| | |   | |__| | (_| | | | | | |  __/
    //|______|_| |_|\__,_|  \____/|_|    \_____|\__,_|_| |_| |_|\___|
    //                                            

    _gui_end_of_game = GUIGridLayout::create();
    _gui_end_of_game->set_rows_and_columns(5, 1);
    _gui_end_of_game->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_end_of_game->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_end_of_game->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _gui_end_of_game->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 256.0F/screen_height));
    _gui_end_of_game->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_end_of_game->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_master->add_item(1, 1, _gui_end_of_game);
    
    std::shared_ptr<GUIGridLayout> gui_layout_end_of_game_1 = GUIGridLayout::create();
    gui_layout_end_of_game_1->set_rows_and_columns(1, 5);
    gui_layout_end_of_game_1->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_end_of_game_1->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    gui_layout_end_of_game_1->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
    gui_layout_end_of_game_1->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
    gui_layout_end_of_game_1->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
    gui_layout_end_of_game_1->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_end_of_game->add_item(1, 0, gui_layout_end_of_game_1);

    
    // Title
    {
        _gui_end_of_game_message = GUIObject::create();
        world->add_node_unique_name(_gui_end_of_game_message);
        _gui_controller->add_child(_gui_end_of_game_message);
        
        _gui_end_of_game->add_item(3,0,_gui_end_of_game_message);
        
        _gui_end_of_game_message->set_label("{TXT_WIN}");
        _gui_end_of_game_message->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _gui_end_of_game_message->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Score
    {
        _gui_end_of_game_score = GUIObject::create();
        world->add_node_unique_name(_gui_end_of_game_score);
        _gui_controller->add_child(_gui_end_of_game_score);
        
        _gui_end_of_game->add_item(2,0,_gui_end_of_game_score);
        
        _gui_end_of_game_score->set_label("{TXT_END_OF_GAME_SCORE}");
        _gui_end_of_game_score->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _gui_end_of_game_score->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }
    
    // Play again
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        gui_layout_end_of_game_1->add_item(0,1,widget);
        
        widget->set_label("{TXT_PLAY_AGAIN}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::restart));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }

    // Quit
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        gui_layout_end_of_game_1->add_item(0,3,widget);
        
        widget->set_label("{TXT_QUIT}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::quit));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }

    //     ______
    //    |  ____|
    //    | |__   _ __ _ __ ___  _ __ ___
    //    |  __| | '__| '__/ _ \| '__/ __|
    //    | |____| |  | | | (_) | |  \__ \
    //    |______|_|  |_|  \___/|_|  |___/
    
    _get_full_version_layout = GUIGridLayout::create();
    _get_full_version_layout->set_rows_and_columns(5, 3);
    _get_full_version_layout->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _get_full_version_layout->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _get_full_version_layout->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
    _get_full_version_layout->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 2.0F * button_size/screen_height));
    _get_full_version_layout->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    if (phone_mode) {
        _get_full_version_layout->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 64.0F/screen_width));
        _get_full_version_layout->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _get_full_version_layout->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 64.0F/screen_width));
    } else {
        _get_full_version_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _get_full_version_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 400.0F/screen_width));
        _get_full_version_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    }

    std::shared_ptr<GUIGridLayout> error_layout_buttons = GUIGridLayout::create();
    error_layout_buttons->set_rows_and_columns(1, 5);
    error_layout_buttons->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    error_layout_buttons->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    error_layout_buttons->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
    error_layout_buttons->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_width));
    error_layout_buttons->set_column_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
    error_layout_buttons->set_column_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _get_full_version_layout->add_item(1, 1, error_layout_buttons);

    
    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _get_full_version_layout->set_border_item(widget, 0.05F, 0.05F);
        
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg_opaque.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }
    
    // Title
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _get_full_version_layout->add_item(3,1,widget);
        
        widget->set_label("{TXT_APP_GET_FULL}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        
    }
    
    
    // Store
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        error_layout_buttons->add_item(0,1,widget);
        
        widget->set_label("{STORE}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::go_to_store_nag));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Dismiss
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        error_layout_buttons->add_item(0,3,widget);
        
        widget->set_label("{DISMISS}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::dismiss_nag));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    _get_full_version_layout->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));

    
    // _____
    //|  __ \                    
    //| |__) |_ _ _   _ ___  ___ 
    //|  ___/ _` | | | / __|/ _ \
    //| |  | (_| | |_| \__ \  __/
    //|_|   \__,_|\__,_|___/\___|
    //

    _gui_pause_button = GUIGridLayout::create();
    _gui_pause_button->set_rows_and_columns(3, 3);
    _gui_pause_button->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_pause_button->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size*0.5F/screen_height));

    if (full_version) {
        _gui_pause_button->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 25.0F/screen_height));
    } else {
        if (phone_mode)
            _gui_pause_button->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 110.0F/screen_height));
        else
            _gui_pause_button->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));
    }

    _gui_pause_button->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 5.0F/screen_width));
    _gui_pause_button->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size*0.5F/screen_width));
    _gui_pause_button->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    // Pause
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_pause_button->add_item(1,1,widget);
        
        widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::show_pause));

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_pause.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }






    _gui_pause = GUIGridLayout::create();
    _gui_pause->set_rows_and_columns(8, 3);
    _gui_pause->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_pause->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
    _gui_pause->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));   // Quit
    _gui_pause->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));   // Sound
    _gui_pause->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));   // Resume
    _gui_pause->set_row_policy(5, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));   // Title
    _gui_pause->set_row_policy(6, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
    _gui_pause->set_row_policy(7, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_pause->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_pause->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 300.0F/screen_width));
    _gui_pause->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _gui_layout_master->add_item(1, 1, _gui_pause);
    
    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_pause->set_border_item(widget, 0.05F, 0.05F);
        
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }

    // Title
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_pause->add_item(5,1,widget);
        
        widget->set_label("{TXT_PAUSE}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Resume
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_pause->add_item(4,1,widget);
        
        widget->set_label("{TXT_RESUME}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::resume));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }

    // Quit
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _gui_pause->add_item(2,1,widget);
        
        widget->set_label("{TXT_QUIT}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::quit));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }

    // Sound button
    {
        _pause_sound = GUIObject::create();
        world->add_node_unique_name(_pause_sound);
        _gui_controller->add_child(_pause_sound);
        
        _gui_pause->add_item(3,1,_pause_sound);

        if (_pause_sound_on)    _pause_sound->set_label("{TXT_SOUND_ON}");
        else                    _pause_sound->set_label("{TXT_SOUND_OFF}");

        _pause_sound->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _pause_sound->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_pause_sound));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _pause_sound->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }






    // _    _ _       _        _____
    //| |  | (_)     | |      / ____|                   
    //| |__| |_  __ _| |__   | (___   ___ ___  _ __ ___ 
    //|  __  | |/ _` | '_ \   \___ \ / __/ _ \| '__/ _ \
    //| |  | | | (_| | | | |  ____) | (_| (_) | | |  __/
    //|_|  |_|_|\__, |_| |_| |_____/ \___\___/|_|  \___|
    //           __/ |                                  
    //          |___/

    DTfloat keyboard_button_size;
    DTfloat keyboard_buttons_width = 1.0F;
    DTfloat keyboard_buttons_height = 1.0F;

    if (phone_mode) {
        keyboard_button_size = 1.0F/12.0F * screen_width;
        keyboard_buttons_width = 1.0F/12.0F;
        keyboard_buttons_height = keyboard_button_size / screen_height;

    } else {
        keyboard_button_size = 64.0F;
        keyboard_buttons_width = 64.0F / screen_width;
        keyboard_buttons_height = 64.0F / screen_height;
    }

    _enter_high_score_layout = GUIGridLayout::create();
    _enter_high_score_rows = GUIGridLayout::create();
    _row1 = GUIGridLayout::create();
    _row2 = GUIGridLayout::create();
    _row3 = GUIGridLayout::create();
    _row4 = GUIGridLayout::create();
    _row5 = GUIGridLayout::create();
    _row_buttons = GUIGridLayout::create();
    
    _enter_high_score_rows->set_rows_and_columns(13,3);

    if (phone_mode) {
        _enter_high_score_rows->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
    } else {
#if DT3_OS == DT3_ANDROID
        _enter_high_score_rows->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 50.0F/screen_height));
#else
        _enter_high_score_rows->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_height));
#endif
    }

    _enter_high_score_rows->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _enter_high_score_rows->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _enter_high_score_rows->set_row_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_height));
    _enter_high_score_rows->set_row_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_height));
    _enter_high_score_rows->set_row_policy(5, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_height));
    _enter_high_score_rows->set_row_policy(6, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_height));
    _enter_high_score_rows->set_row_policy(7, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_height));
    _enter_high_score_rows->set_row_policy(8, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _enter_high_score_rows->set_row_policy(9, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, button_size/screen_height));
    _enter_high_score_rows->set_row_policy(10, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _enter_high_score_rows->set_row_policy(11, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    if (phone_mode) {
        _enter_high_score_rows->set_row_policy(12, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 150.0F/screen_height));
    } else {
#if DT3_OS == DT3_ANDROID
        _enter_high_score_rows->set_row_policy(12, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 50.0F/screen_height));
#else
        _enter_high_score_rows->set_row_policy(12, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_height));
#endif
    }

    if (phone_mode) {
        _enter_high_score_rows->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 15/screen_width));
        _enter_high_score_rows->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _enter_high_score_rows->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 15/screen_width));
    } else {
        _enter_high_score_rows->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _enter_high_score_rows->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 800/screen_width));
        _enter_high_score_rows->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    }

    _enter_high_score_rows->add_item(7,1, _row1);
    _enter_high_score_rows->add_item(6,1, _row2);
    _enter_high_score_rows->add_item(5,1, _row3);
    _enter_high_score_rows->add_item(4,1, _row4);
    _enter_high_score_rows->add_item(3,1, _row5);
    _enter_high_score_rows->add_item(1,1, _row_buttons);
    
    
    
    _enter_high_score_layout->set_rows_and_columns(1,1);
    _enter_high_score_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _enter_high_score_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    
    _enter_high_score_layout->add_item(0,0, _enter_high_score_rows);
    
    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _enter_high_score_rows->set_border_item(widget, 0.02F, 0.05F);
        
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }
    
    _row1->set_rows_and_columns(1,13);
    _row1->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row1->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row1->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(5, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(6, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(7, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(8, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(9, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(10, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(11, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row1->set_column_policy(12, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    
    _row1->add_item(0,1,make_key("1",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,2,make_key("2",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,3,make_key("3",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,4,make_key("4",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,5,make_key("5",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,6,make_key("6",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,7,make_key("7",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,8,make_key("8",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,9,make_key("9",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,10,make_key("0",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row1->add_item(0,11,make_key("<",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);



    _row2->set_rows_and_columns(1,12);
    _row2->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row2->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row2->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(5, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(6, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(7, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(8, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(9, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(10, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row2->set_column_policy(11, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    _row2->add_item(0,1,make_key("Q",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,2,make_key("W",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,3,make_key("E",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,4,make_key("R",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,5,make_key("T",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,6,make_key("Y",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,7,make_key("U",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,8,make_key("I",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,9,make_key("O",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row2->add_item(0,10,make_key("P",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);



    _row3->set_rows_and_columns(1,11);
    _row3->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row3->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row3->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(5, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(6, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(7, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(8, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(9, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row3->set_column_policy(10, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    _row3->add_item(0,1,make_key("A",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row3->add_item(0,2,make_key("S",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row3->add_item(0,3,make_key("D",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row3->add_item(0,4,make_key("F",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row3->add_item(0,5,make_key("G",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row3->add_item(0,6,make_key("H",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row3->add_item(0,7,make_key("J",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row3->add_item(0,8,make_key("K",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row3->add_item(0,9,make_key("L",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);



    _row4->set_rows_and_columns(1,9);
    _row4->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row4->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row4->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row4->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row4->set_column_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row4->set_column_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row4->set_column_policy(5, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row4->set_column_policy(6, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row4->set_column_policy(7, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, keyboard_buttons_width));
    _row4->set_column_policy(8, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    _row4->add_item(0,1,make_key("Z",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row4->add_item(0,2,make_key("X",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row4->add_item(0,3,make_key("C",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row4->add_item(0,4,make_key("V",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row4->add_item(0,5,make_key("B",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row4->add_item(0,6,make_key("N",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    _row4->add_item(0,7,make_key("M",keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
    
    
    _row5->set_rows_and_columns(1,3);
    _row5->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row5->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row5->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 350.0F/screen_width));
    _row5->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    _row5->add_item(0,1,make_key_space(keyboard_buttons_width, keyboard_buttons_height),GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);


    _row_buttons->set_rows_and_columns(1,5);
    _row_buttons->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row_buttons->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row_buttons->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_width));
    _row_buttons->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _row_buttons->set_column_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 200.0F/screen_width));
    _row_buttons->set_column_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    
    
    // Title
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _enter_high_score_rows->add_item(11, 1, widget);
        
        widget->set_label("{TXT_ENTER_HIGH_SCORE}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::switch_options_min_bid, (DTshort) 5));

        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        
    }

    // Name text box
    {
        _text_box = GUIObject::create();
        world->add_node_unique_name(_text_box);
        _gui_controller->add_child(_text_box);
        
        _enter_high_score_rows->add_item(9, 1, _text_box);
        
        _text_box->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        _text_box->set_no_focus(true);
        _text_box->set_width(450.0F / screen_width);
        _text_box->set_height(50.0F / screen_height);

        _text_box->set_label("{FMT_BUTTON}{APP_USER_NAME}");

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        _text_box->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }
    
    
    // Submit
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _row_buttons->add_item(0, 3, widget);
        
        widget->set_label("{TXT_SUBMIT}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::submit));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }

    // Skip
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        _gui_controller->add_child(widget);
        
        _row_buttons->add_item(0, 1, widget);
        
        widget->set_label("{TXT_SKIP}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserGameController::skip));

        // Drawing
        std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
        widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
    }
    
    _enter_high_score_layout->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));
    
#if DT3_OS != DT3_ANDROID
    //  _____            _       _   __  __          _ _
    // / ____|          (_)     | | |  \/  |        | (_)      
    //| (___   ___   ___ _  __ _| | | \  / | ___  __| |_  __ _ 
    // \___ \ / _ \ / __| |/ _` | | | |\/| |/ _ \/ _` | |/ _` |
    // ____) | (_) | (__| | (_| | | | |  | |  __/ (_| | | (_| |
    //|_____/ \___/ \___|_|\__,_|_| |_|  |_|\___|\__,_|_|\__,_|

    if (!phone_mode) {
        _social_media_layout = GUIGridLayout::create();
        _social_media_layout->set_rows_and_columns(3,5);
        _social_media_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_height));
        _social_media_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_height));
        _social_media_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _social_media_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _social_media_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_width));
        _social_media_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_width));
        _social_media_layout->set_column_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 30.0F/screen_width));
        _social_media_layout->set_column_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_width));

        // Facebook
        {
            std::shared_ptr<GUIObject> widget = GUIObject::create();
            world->add_node_unique_name(widget);
            _gui_controller->add_child(widget);
            
            _social_media_layout->add_item(1, 1, widget);
            
            widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
            
            // Interaction 
            std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
            widget->add_component(widget_interaction);
            widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_facebook));

            // Drawing
            std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
            widget->add_component(widget_drawing);
            
            widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_facebook.mat}")));
            widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        }

        // Twitter
        {
            std::shared_ptr<GUIObject> widget = GUIObject::create();
            world->add_node_unique_name(widget);
            _gui_controller->add_child(widget);
            
            _social_media_layout->add_item(1, 3, widget);
            
            widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
            
            // Interaction 
            std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
            widget->add_component(widget_interaction);
            widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_twitter));

            // Drawing
            std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
            widget->add_component(widget_drawing);
            
            widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_twitter.mat}")));
            widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        }
        
        _social_media_layout->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));
    }
#endif
    
    _gui_pause_button->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));
    _gui_layout_messages->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));
    _gui_layout_turn->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));
    _gui_layout_master->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));
    _gui_layout_scoreboard->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));
    
#endif

    //ThreadMainTaskQueue::add_task(make_latent_call(this, &KaiserGameController::set_state, STATE_DEALING, _dealer), 0.0F, 0.5F);
}

void KaiserGameController::remove_from_world (void)
{
    world()->unregister_for_tick(this, make_callback(this, &type::tick));

    GameController::remove_from_world();
}

//==============================================================================
//==============================================================================

} // DT3

