//==============================================================================
///	
///	File: KaiserPlayer.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserPlayer.hpp"
#include <algorithm>

//==============================================================================
//==============================================================================

namespace DT3 {
    
//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserPlayer::KaiserPlayer (void)
{

}
	
KaiserPlayer::KaiserPlayer (const KaiserPlayer &rhs)
    :   BaseClass(rhs)
{

}

KaiserPlayer& KaiserPlayer::operator = (const KaiserPlayer &rhs)
{
    // Make sure we are not assigning the class to itself
    if (&rhs != this) {
		BaseClass::operator = (rhs);
    }
    return (*this);
}
		
KaiserPlayer::~KaiserPlayer (void)
{

}

//==============================================================================
//==============================================================================

struct SorterHand {

    DTboolean operator() (const std::shared_ptr<KaiserObjectCard> &a, const std::shared_ptr<KaiserObjectCard> &b)
    {
        DTint as,bs;

        switch (a->suit()) {
            case SUIT_CLUB:     as = 0; break;
            case SUIT_DIAMOND:  as = 1; break;
            case SUIT_HEART:    as = 3; break;
            case SUIT_SPADE:    as = 2; break;
        }

        switch (b->suit()) {
            case SUIT_CLUB:     bs = 0; break;
            case SUIT_DIAMOND:  bs = 1; break;
            case SUIT_HEART:    bs = 3; break;
            case SUIT_SPADE:    bs = 2; break;
        }


        DTuint aa = a->value() | as << 16;
        DTuint bb = b->value() | bs << 16;

        return aa > bb;
    }

};

void KaiserPlayer::sort (void)
{
    std::sort(_cards.begin(), _cards.end(), SorterHand());
}

void KaiserPlayer::swap (   const std::shared_ptr<KaiserObjectCard> &old_card,
                            const std::shared_ptr<KaiserObjectCard> &new_card)
{
    auto i = std::find(_cards.begin(), _cards.end(), old_card);
    ASSERT(i != _cards.end());

    *i = new_card;
}

//==============================================================================
//==============================================================================

void KaiserPlayer::add_card (const std::shared_ptr<KaiserObjectCard> &card)
{
    _cards.push_back(card);
}

void KaiserPlayer::remove_card (const std::shared_ptr<KaiserObjectCard> &card)
{
    auto i = std::find(_cards.begin(), _cards.end(), card);
    if (i != _cards.end())
        _cards.erase(i);
}

//==============================================================================
//==============================================================================

void KaiserPlayer::add_trick (const std::shared_ptr<KaiserObjectCard> played_cards[4])
{
    Trick t;
    t._played_cards[0] = played_cards[0];
    t._played_cards[1] = played_cards[1];
    t._played_cards[2] = played_cards[2];
    t._played_cards[3] = played_cards[3];
    
    _tricks.push_back(t);
}

//==============================================================================
//==============================================================================

DTshort KaiserPlayer::score (void)
{
    DTshort score = 0;
    
    for (auto &i : _tricks) {
        score += 1;
        
        // Check for 5 hearts
        for (DTint j = 0; j < 4; ++j) {
            if (i._played_cards[j]->value() == CARD_5)
                score += 5;
            
            if (i._played_cards[j]->value() == CARD_3)
                score -= 3;

        }
        
    }
    
    return score;
}

//==============================================================================
//==============================================================================

DTboolean KaiserPlayer::has_5_hearts (void)
{

    for (auto &i : _tricks) {

        // Check for 5 hearts
        for (DTint j = 0; j < 4; ++j) {
            if (i._played_cards[j]->value() == CARD_5)
                return true;
        }
        
    }

    return false;
}

//==============================================================================
//==============================================================================

void KaiserPlayer::limit (Suit led_suit)
{
    DTshort count = 0;
    
    // Check if player has cards of suit
    for (auto &i : _cards) {
    
        i->set_can_use(true);
        
        if (i->suit() == led_suit)
            ++count;
        
    }
    
    if (count > 0) {
    
        for (auto &i : _cards) {
        
            if (i->suit() != led_suit)
                i->set_can_use(false);
            
        }
    }
    
}

void KaiserPlayer::clear_limits (void)
{
    for (auto &i : _cards) {
        i->set_can_use(true);
    }
}

//==============================================================================
//==============================================================================

} // DT3

