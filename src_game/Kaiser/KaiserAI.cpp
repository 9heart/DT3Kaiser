//==============================================================================
///	
///	File: KaiserAI.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserAI.hpp"
#include "Kaiser/KaiserObjectCard.hpp"
#include "DT3Core/Types/Math/MoreMath.hpp"
#include "DT3Core/Types/Utility/ConsoleStream.hpp"
#include "DT3Core/System/StaticInitializer.hpp"
#include <algorithm>

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
//==============================================================================

GLOBAL_STATIC_INITIALIZATION(0,KaiserAI::initialize_static())
GLOBAL_STATIC_DESTRUCTION(0,KaiserAI::uninitialize_static())

//==============================================================================
//==============================================================================

DTfloat BID_WEIGHTS[13];
const DTfloat BID_MUL = 6.5F;

//==============================================================================
//==============================================================================

void KaiserAI::initialize_static (void)
{
    DTfloat s = 0.0F;
    
    // Build normalized exponential curve
    for (DTint i = 0; i < 13; ++i) {
        BID_WEIGHTS[i] = i*i;
        s += BID_WEIGHTS[i];
    }

    // Normalize values
    for (DTint i = 0; i < 13; ++i) {
        BID_WEIGHTS[i] /= s;
    }
}

void KaiserAI::uninitialize_static (void)
{

}

//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserAI::KaiserAI (void)
{
    
}
		
KaiserAI::KaiserAI (const KaiserAI &rhs)
    :   BaseClass         (rhs)
{

}

KaiserAI & KaiserAI::operator = (const KaiserAI &rhs)
{
    // Make sure we are not assigning the class to itself
    if (&rhs != this) { 
		BaseClass::operator = (rhs);
    }
    return (*this);
}
			
KaiserAI::~KaiserAI (void)
{	

}

//==============================================================================
//==============================================================================

struct SorterValue {

    DTboolean operator() (const std::shared_ptr<KaiserObjectCard> &a, const std::shared_ptr<KaiserObjectCard> &b) {

        DTint as,bs;

        if (a->suit() == trump)                     as = 4;
        else if (a->suit() == first_player_suit)    as = 3;
        else if (a->suit() == hint)                 as = 1;
        else if (a->suit() == run)                  as = 2;
        else                                        as = 0;

        if (b->suit() == trump)                     bs = 4;
        else if (b->suit() == first_player_suit)    bs = 3;
        else if (b->suit() == hint)                 bs = 1;
        else if (b->suit() == run)                  bs = 2;
        else                                        bs = 0;

        DTint av,bv;

        if (invert_spades && a->suit() == SUIT_SPADE) {
            av = (as << 16) | ( 12 - (a->value()-6));
        } else {
            av = (as << 16) | a->value();
        }

        if (invert_spades && b->suit() == SUIT_SPADE) {
            bv = (bs << 16) | ( 12 - (b->value()-6));
        } else {
            bv = (bs << 16) | b->value();
        }

        return av < bv;

    }

    Suit first_player_suit;
    Suit trump;
    Suit hint;
    Suit run;

    DTboolean invert_spades;    // For playing first card
};

struct SorterPass {

    DTboolean operator() (const std::shared_ptr<KaiserObjectCard> &a, const std::shared_ptr<KaiserObjectCard> &b) {

        DTfloat av = a->value();
        DTfloat bv = b->value();

        if (a->suit() == SUIT_HEART && a->value() == CARD_5)    av = 1000.0F;
        if (b->suit() == SUIT_HEART && b->value() == CARD_5)    bv = 1000.0F;

        if (a->suit() == SUIT_SPADE && a->value() == CARD_3)    av = 1000.0F;
        if (b->suit() == SUIT_SPADE && b->value() == CARD_3)    bv = 1000.0F;

        return av < bv;
    }

};

//==============================================================================
//==============================================================================

std::shared_ptr<KaiserObjectCard> KaiserAI::find_card (DTshort value, Suit suit, const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards)
{
    for (auto &i : player_cards) {
        if (i->suit() == suit && i->value() == value && i->can_use())
            return i;
    }

    return NULL;
}

DTboolean KaiserAI::has_card (DTshort value, Suit suit, const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards)
{
    for (auto &i : player_cards) {
        if (i->suit() == suit && i->value() == value)
            return true;
    }

    return false;
}

//==============================================================================
//==============================================================================

DTfloat KaiserAI::weighted_sum_for_suit(Suit s, const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards)
{
    DTfloat sum = 0.0F;
    
    for (auto &i : player_cards) {
        if (i->suit() == s) {
            sum += BID_WEIGHTS[i->value()] * BID_MUL;
        }
    }
    
    return sum;
}

//==============================================================================
//==============================================================================

DTfloat KaiserAI::num_for_suit(Suit s, const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards)
{
    DTfloat sum = 0.0F;
    
    for (auto &i : player_cards) {
        if (i->suit() == s) {
            sum += 1.0F;
        }
    }
    
    return sum;
}

//==============================================================================
//==============================================================================

void KaiserAI::pass (   Player p,
                        const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                        std::shared_ptr<KaiserObjectCard> &c0,
                        std::shared_ptr<KaiserObjectCard> &c1   )
{
    std::vector<std::shared_ptr<KaiserObjectCard>> cards = player_cards;

    // Sort cards by odds of winning_player
    SorterPass s;
    std::sort(cards.begin(), cards.end(), s);

    c0 = cards[cards.size()-1];
    c1 = cards[cards.size()-2];
}

//==============================================================================
//==============================================================================

void KaiserAI::begin (Player p)
{
    // Reset hand
    _did_hint = false;
    _hinted_suit = SUIT_UNDEFINED;
    _run_suit = SUIT_UNDEFINED;
    _did_play_3 = false;

    _highest_card[SUIT_CLUB] = CARD_ACE;
    _highest_card[SUIT_HEART] = CARD_ACE;
    _highest_card[SUIT_DIAMOND] = CARD_ACE;
    _highest_card[SUIT_SPADE] = CARD_ACE;
}

//==============================================================================
//==============================================================================


Suit KaiserAI::trump_suit  (    Player p,
                                const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                                const std::shared_ptr<KaiserBid> &top_bid)
{
    // Sum up different suits
    DTfloat suits[4];
    suits[SUIT_CLUB] = weighted_sum_for_suit(SUIT_CLUB, player_cards);
    suits[SUIT_DIAMOND] = weighted_sum_for_suit(SUIT_DIAMOND, player_cards);
    suits[SUIT_HEART] = weighted_sum_for_suit(SUIT_HEART, player_cards);
    suits[SUIT_SPADE] = weighted_sum_for_suit(SUIT_SPADE, player_cards);

    DTfloat nums[4];
    nums[SUIT_CLUB] = num_for_suit(SUIT_CLUB, player_cards);
    nums[SUIT_DIAMOND] = num_for_suit(SUIT_DIAMOND, player_cards);
    nums[SUIT_HEART] = num_for_suit(SUIT_HEART, player_cards);
    nums[SUIT_SPADE] = num_for_suit(SUIT_SPADE, player_cards);

    // Check for roughly equal suits
    Suit max_suit = SUIT_NO;
    DTfloat max_num = 0.0F;
    for (DTint i = 0; i < 4; ++i) {
        if (nums[i] > max_num) {
            max_suit = (Suit) i;
            max_num = nums[i];
            
        } else if (nums[i] == max_num) {
        
            if (suits[i] > suits[max_suit]) {
                max_suit = (Suit) i;
                max_num = nums[i];
            }
            
        }
    }

    return (Suit) max_suit;
}

//==============================================================================
//==============================================================================

std::shared_ptr<KaiserBid> KaiserAI::bid (  Player p,
                                            Player dealer,
                                            const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                                            DTshort minimum_bid,
                                            DTboolean forced_bid,
                                            const std::shared_ptr<KaiserBid> &top_bid)
{
//    if (!forced_bid) return std::shared_ptr<KaiserBid>(new KaiserBid()); // Pass

    // Kevin's trick: Player across from dealer can signal dealer
    if ((dealer == PLAYER_0 && p == PLAYER_2) ||
        (dealer == PLAYER_1 && p == PLAYER_3) ||
        (dealer == PLAYER_2 && p == PLAYER_0) ||
        (dealer == PLAYER_3 && p == PLAYER_1) ) {

        // Special case having Ace or 5 of hearts
        for (DTint i = 0; i < player_cards.size(); ++i) {

            if (player_cards[i]->suit() == SUIT_HEART && player_cards[i]->value() == CARD_5)
                return std::shared_ptr<KaiserBid>(new KaiserBid(minimum_bid, false));

            if (player_cards[i]->suit() == SUIT_HEART && player_cards[i]->value() == CARD_ACE)
                return std::shared_ptr<KaiserBid>(new KaiserBid(minimum_bid, false));
        }
    }


    // Sum up different suits
    DTfloat suits[4];
    suits[SUIT_CLUB] = weighted_sum_for_suit(SUIT_CLUB, player_cards);
    suits[SUIT_DIAMOND] = weighted_sum_for_suit(SUIT_DIAMOND, player_cards);
    suits[SUIT_HEART] = weighted_sum_for_suit(SUIT_HEART, player_cards);
    suits[SUIT_SPADE] = weighted_sum_for_suit(SUIT_SPADE, player_cards);

    DTfloat bid_num;

    if (forced_bid) {
        bid_num = minimum_bid;
    } else {
        bid_num = std::floor(suits[SUIT_CLUB] + suits[SUIT_DIAMOND] + suits[SUIT_HEART] + suits[SUIT_SPADE] + 0.5);

        if (bid_num > 12)
            bid_num = 12;

        if (bid_num < minimum_bid)
            return std::shared_ptr<KaiserBid>(new KaiserBid()); // Pass
    }

    
    DTfloat nums[4];
    nums[SUIT_CLUB] = num_for_suit(SUIT_CLUB, player_cards);
    nums[SUIT_DIAMOND] = num_for_suit(SUIT_DIAMOND, player_cards);
    nums[SUIT_HEART] = num_for_suit(SUIT_HEART, player_cards);
    nums[SUIT_SPADE] = num_for_suit(SUIT_SPADE, player_cards);

    Suit max_suit = trump_suit  (p,player_cards,top_bid);

    if (!forced_bid) {
        // Kevin: If not bidding hearts and more then two hearts, pass
        if (nums[SUIT_HEART] >= 3 && max_suit != SUIT_HEART) {
            return std::shared_ptr<KaiserBid>(new KaiserBid());
        }
    }


    DTboolean no = false;

    if (    (nums[SUIT_CLUB] >= 4 && has_card(CARD_ACE, SUIT_CLUB, player_cards) && has_card(CARD_KING, SUIT_CLUB, player_cards)) ||
            (nums[SUIT_CLUB] >= 5 && has_card(CARD_ACE, SUIT_CLUB, player_cards) && has_card(CARD_QUEEN, SUIT_CLUB, player_cards)) ) {
        _run_suit = SUIT_CLUB;
        no = true;
    }

    if (    (nums[SUIT_DIAMOND] >= 4 && has_card(CARD_ACE, SUIT_DIAMOND, player_cards) && has_card(CARD_KING, SUIT_DIAMOND, player_cards)) ||
            (nums[SUIT_DIAMOND] >= 5 && has_card(CARD_ACE, SUIT_DIAMOND, player_cards) && has_card(CARD_QUEEN, SUIT_DIAMOND, player_cards)) ) {
        _run_suit = SUIT_DIAMOND;
        no = true;
    }

    if (    (nums[SUIT_SPADE] >= 4 && has_card(CARD_ACE, SUIT_SPADE, player_cards) && has_card(CARD_KING, SUIT_SPADE, player_cards)) ||
            (nums[SUIT_SPADE] >= 5 && has_card(CARD_ACE, SUIT_SPADE, player_cards) && has_card(CARD_QUEEN, SUIT_SPADE, player_cards)) ) {
        _run_suit = SUIT_SPADE;
        no = true;
    }

    if (    (has_card(CARD_ACE, SUIT_HEART, player_cards) && has_card(CARD_KING, SUIT_HEART, player_cards)) ||
            (nums[SUIT_HEART] >= 4 && has_card(CARD_ACE, SUIT_HEART, player_cards) && has_card(CARD_QUEEN, SUIT_HEART, player_cards)) ) {
        _run_suit = SUIT_HEART;
        no = true;
    }

    DTshort bid_val = (DTshort) (bid_num + 0.5F);

    // Kevin: Clamp to minimum_bid if hand not good enough
    if (!no && bid_val > minimum_bid && nums[max_suit] < 3) {
        bid_val = minimum_bid;
    }

    std::shared_ptr<KaiserBid> bid = std::shared_ptr<KaiserBid>(new KaiserBid( bid_val, no));

    if (!forced_bid) {
        if ( (p == dealer) && (*bid > *top_bid))
            bid = std::shared_ptr<KaiserBid>(new KaiserBid( top_bid->bid(), top_bid->is_no())); // Take the bid
    }

    return bid;
}

//==============================================================================
//==============================================================================

void KaiserAI::trump (  Player p,
                        const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                        DTboolean forced_bid,
                        const std::shared_ptr<KaiserBid> &top_bid)
{
    // Check for roughly equal suits
    Suit max_suit = trump_suit  (p,player_cards,top_bid);
    top_bid->set_trump((Suit) max_suit);
}

//==============================================================================
//==============================================================================

std::string KaiserAI::print_card  (const std::shared_ptr<KaiserObjectCard> &card)
{
    if (!card)
        return "none";

    std::string c;

    switch (card->value()) {
        case CARD_ACE: c = "ace"; break;
        case CARD_KING: c = "king"; break;
        case CARD_QUEEN: c = "queen"; break;
        case CARD_JACK:  c = "jack"; break;
        case CARD_10:  c = "10"; break;
        case CARD_9:  c = "9"; break;
        case CARD_8:  c = "8"; break;
        case CARD_7:  c = "7"; break;
        case CARD_6:  c = "6"; break;
        case CARD_5:  c = "5"; break;
        case CARD_4:  c = "4"; break;
        case CARD_3:  c = "3"; break;
        case CARD_2:  c = "2"; break;

    }

    c += " of ";

    switch (card->suit()) {
        case SUIT_CLUB:     c += "clubs";       break;
        case SUIT_DIAMOND:  c += "diamonds";    break;
        case SUIT_HEART:    c += "hearts";      break;
        case SUIT_SPADE:    c += "spades";      break;
    }

    return c;
}

//==============================================================================
//==============================================================================

std::shared_ptr<KaiserObjectCard> KaiserAI::play    (   Player p,
                                                        Player first_player,
                                                        const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                                                        const std::shared_ptr<KaiserObjectCard> played_cards[4],
                                                        const std::shared_ptr<KaiserBid> &top_bid)
{
    // Check for special cards played (3 & 5) for the current value of the trick
    DTfloat hand_value = 1.0F;
    
    for (DTint i = 0; i < 4; ++i) {
        if (!played_cards[i])   continue;
        
        if (played_cards[i]->value() == CARD_3)
            hand_value -= 3.0F;

        if (played_cards[i]->value() == CARD_5)
            hand_value += 5.0F;
    }

    DTboolean all_cards_played =   ((played_cards[0] ? 1 : 0) +
                                    (played_cards[1] ? 1 : 0) +
                                    (played_cards[2] ? 1 : 0) +
                                    (played_cards[3] ? 1 : 0)) == 3;

    //
    // Get first suit and trump
    //

    Suit first_player_suit = played_cards[first_player] ? played_cards[first_player]->suit() : SUIT_UNDEFINED;
    Suit trump = top_bid->trump();

    //
    // Get playable cards plus 5 and 3
    //

    std::vector<std::shared_ptr<KaiserObjectCard>> cards;
    std::shared_ptr<KaiserObjectCard>   three_spades;
    std::shared_ptr<KaiserObjectCard>   five_hearts;

    for (DTint i = 0; i < player_cards.size(); ++i) {
        if (player_cards[i]->can_use()) {
            cards.push_back(player_cards[i]);

            if (player_cards[i]->value() == CARD_5)
                five_hearts = player_cards[i];
            
            if (player_cards[i]->value() == CARD_3)
                three_spades = player_cards[i];

        }
    }

    //
    // Don't one-up partner
    //

    Player other_p;
    if (p == PLAYER_0)  other_p = PLAYER_2;
    if (p == PLAYER_1)  other_p = PLAYER_3;
    if (p == PLAYER_2)  other_p = PLAYER_0;
    if (p == PLAYER_3)  other_p = PLAYER_1;

    DTint one_up_thresh = ( (first_player_suit == SUIT_HEART) && five_hearts) ? 2 : 1;

    if (    played_cards[other_p] &&
            played_cards[other_p]->value() < CARD_ACE &&
            cards.size() > one_up_thresh) {

        auto i = find_card( played_cards[other_p]->value() + 1,
                            played_cards[other_p]->suit(),
                            cards);

        auto e = std::remove(cards.begin(), cards.end(), i);
        cards.erase(e, cards.end());

    }

    //
    // Sort cards by odds of winning_player
    //

    SorterValue s;
    s.trump = trump;
    s.first_player_suit = first_player_suit;
    s.hint = _hinted_suit;
    s.run = _run_suit;
    s.invert_spades = (p == first_player) && !_did_play_3 && !three_spades;

    std::sort(cards.begin(), cards.end(), s);

    LOG_MESSAGE << "Sorted cards: ";
    for (DTint i = 0; i < cards.size(); ++i) {
        LOG_MESSAGE << " AI Card: " << print_card(cards[i]);
    }

    //
    // Find minimum winning_player card.
    //

    std::shared_ptr<KaiserObjectCard> min_winning_card;
    DTint other_rank = 0;

    if (p == PLAYER_0 || p == PLAYER_2) {
        other_rank =    MoreMath::max(  (played_cards[PLAYER_1] ? rank_card(played_cards[PLAYER_1], trump, first_player_suit, SUIT_UNDEFINED) : 0),
                                        (played_cards[PLAYER_3] ? rank_card(played_cards[PLAYER_3], trump, first_player_suit, SUIT_UNDEFINED) : 0)  );
    } else {
        other_rank =    std::max(   (played_cards[PLAYER_0] ? rank_card(played_cards[PLAYER_0], trump, first_player_suit, SUIT_UNDEFINED) : 0),
                                    (played_cards[PLAYER_2] ? rank_card(played_cards[PLAYER_2], trump, first_player_suit, SUIT_UNDEFINED) : 0)  );
    }

    if (other_rank > 0) {

        for (DTint i = 0; i < cards.size(); ++i) {
            // Skip 5 of hearts and 3 of spades
            if (cards[i] == five_hearts || cards[i] == three_spades)
                continue;

            if (other_rank < rank_card(cards[i], trump, first_player_suit, SUIT_UNDEFINED)) {
                min_winning_card = cards[i];
                break;
            }
        }
        
    }

    //
    // Check who is currently winning_player
    //

    Player winning_player = PLAYER_0;
    DTint winning_player_rank = 0;

    for (DTshort p = 0; p < 4; ++p) {
        DTint r = rank_card (played_cards[p], trump, first_player_suit, SUIT_UNDEFINED);

        if (r > winning_player_rank) {
            winning_player_rank = r;
            winning_player = (Player) p;
        }
    }

    std::shared_ptr<KaiserObjectCard> winning_card;

    //
    // Is partner winning_player
    //

    DTboolean we_are_winning = false;
    DTboolean winning_player_won = false;

    if (played_cards[winning_player]) {
        if (p == PLAYER_0 && winning_player == PLAYER_2)   we_are_winning = true;
        if (p == PLAYER_1 && winning_player == PLAYER_3)   we_are_winning = true;
        if (p == PLAYER_2 && winning_player == PLAYER_0)   we_are_winning = true;
        if (p == PLAYER_3 && winning_player == PLAYER_1)   we_are_winning = true;

        // If the winning player played the highest card left...
        if (    (played_cards[winning_player]->value() == _highest_card[played_cards[winning_player]->suit()]) &&
                (top_bid->is_no() || played_cards[winning_player]->suit() == top_bid->trump())) {
            winning_player_won = true;
        }

        winning_card = played_cards[winning_player];
    }


    // Remove min winning card if partner alread won
    if (we_are_winning && winning_player_won)
        min_winning_card.reset();

    //
    // Best cards
    //

    std::shared_ptr<KaiserObjectCard> worst_non_trump_card;
    std::shared_ptr<KaiserObjectCard> worst_trump_card;
    std::shared_ptr<KaiserObjectCard> best_non_trump_card;
    std::shared_ptr<KaiserObjectCard> best_trump_card;

    // Find worst non-trump card
    for (DTsize i = cards.size()-1; i >= 0; --i) {

        // Skip 5 of hearts and 3 of spades
        if (cards[i] == five_hearts || cards[i] == three_spades)
            continue;

        if (cards[i]->suit() != trump) {
            worst_non_trump_card = cards[i];
        }

        if (cards[i]->suit() == trump) {
            worst_trump_card = cards[i];
        }

    }


    // Find best non-trump card
    for (DTsize i = 0; i < cards.size(); ++i) {

        // Skip 5 of hearts and 3 of spades
        if (cards[i] == five_hearts || cards[i] == three_spades)
            continue;

        if (cards[i]->suit() != trump) {
            best_non_trump_card = cards[i];
        }

        if (cards[i]->suit() == trump) {
            best_trump_card = cards[i];
        }

    }

    //
    // Kevins hint trick. Only player 2 does it
    //

    std::shared_ptr<KaiserObjectCard> hinted_card;
    if (!_did_hint && top_bid->is_no() && p == PLAYER_2) {

        // Check for strongest suit. Coun't backwards from ace and figure out the strnigest suit.
        DTshort strong_suit_num = 0;
        Suit strong_suit = SUIT_UNDEFINED;

        for (DTshort s = 0; s < 4; ++s) {
            if (s == first_player_suit)
                continue;

            DTshort suit_num = 0;

            for (DTshort v = CARD_ACE; v >= 0; --v) {
                std::shared_ptr<KaiserObjectCard> c = find_card(v, (Suit) s, player_cards);


                if (c) {
                    ++suit_num;

                    if (suit_num > strong_suit_num) {
                        strong_suit_num = suit_num;
                        strong_suit = (Suit) s;
                    }

                } else {
                    break;
                }
            }

        }

        // Scan for lowest card
        if (strong_suit != SUIT_NO) {

            for (DTshort v = CARD_7; v <= CARD_ACE; ++v) {
                std::shared_ptr<KaiserObjectCard> c = find_card(v, strong_suit, player_cards);

                if (c) {
                    hinted_card = c;
                    break;
                }
            }

        }

        // Remove if ace
        if (hinted_card && hinted_card->value() == CARD_ACE)
            hinted_card.reset();

    }


    LOG_MESSAGE << " AI: best_non_trump_card  " << print_card(best_non_trump_card);
    LOG_MESSAGE << " AI: best_trump_card      " << print_card(best_trump_card);
    LOG_MESSAGE << " AI: worst_non_trump_card " << print_card(worst_non_trump_card);
    LOG_MESSAGE << " AI: worst_trump_card     " << print_card(worst_trump_card);
    LOG_MESSAGE << " AI: min_winning_card     " << print_card(min_winning_card);
    LOG_MESSAGE << " AI: hinted_card          " << print_card(hinted_card);
    LOG_MESSAGE << " AI: current winning card " << print_card(winning_card);

    if (we_are_winning)     LOG_MESSAGE << " AI: We are winning";
    if (winning_player_won) LOG_MESSAGE << " AI: Winning player won";


    // We are playing first
    if (p == first_player) {

        // Choose best card
        if (best_non_trump_card) {
            LOG_MESSAGE << "Play S";
            return best_non_trump_card;
        } else if (best_trump_card) {
            LOG_MESSAGE << "Play T";
            return best_trump_card;
        } else {
            LOG_MESSAGE << "Play U";
            return cards.front();
        }

    // If we're going to lose points on this hand
    } else if (hand_value < 0.0) {

        // Choose worst card
        if (hinted_card) {
            LOG_MESSAGE << "Play A";
            _did_hint = true;
            return hinted_card;
        } else if (worst_non_trump_card) {
            LOG_MESSAGE << "Play B";
            return worst_non_trump_card;
        } else if (worst_trump_card) {
            LOG_MESSAGE << "Play C";
            return worst_trump_card;
        } else {
            LOG_MESSAGE << "Play D";
            return cards.front();
        }

    } else if (hand_value > 1.0F && min_winning_card && !winning_player_won) {

        // Choose best card
        if (best_trump_card) {
            LOG_MESSAGE << "Play E";
            return best_trump_card;
        } else if (best_non_trump_card) {
            LOG_MESSAGE << "Play F";
            return best_non_trump_card;
        } else {
            LOG_MESSAGE << "Play G";
            return cards.front();
        }

    // If partner plays ace of hearts, Throw five
    } else if ( five_hearts && played_cards[other_p] && played_cards[other_p]->suit() == SUIT_HEART && played_cards[other_p]->value() == CARD_ACE && we_are_winning) {
        LOG_MESSAGE << "Play YY";
        return five_hearts;

    // If partner played and they are winning, feed the three
    } else if ( three_spades && !we_are_winning && played_cards[other_p] &&
                ( (top_bid->trump() != SUIT_SPADE) ||
                (winning_card && (winning_card->suit() == SUIT_SPADE))) ) {
        LOG_MESSAGE << "Play XX";
        return three_spades;


    // If we are the last to play a card or if player already won
    } else if (all_cards_played || winning_player_won) {

        // Check if partner won, choose worst card
        if (we_are_winning) {

            if (five_hearts) {
                LOG_MESSAGE << "Play H";
                return five_hearts;
            } else {
                // Choose worst card
                if (hinted_card) {
                    LOG_MESSAGE << "Play I";
                    _did_hint = true;
                    return hinted_card;
                } else if (worst_non_trump_card) {
                    LOG_MESSAGE << "Play J";
                    return worst_non_trump_card;
                } else if (worst_trump_card) {
                    LOG_MESSAGE << "Play K";
                    return worst_trump_card;
                } else {
                    LOG_MESSAGE << "Play L";
                    return cards.front();
                }
            }

        // Choose minimal best card, priority non-trump
        } else {

            if (three_spades && ( (top_bid->trump() != SUIT_SPADE) || (winning_card && (winning_card->suit() == SUIT_SPADE))) ) {
                LOG_MESSAGE << "Play M";
                return three_spades;

            } else if (min_winning_card && !(we_are_winning && winning_player_won)) {
                LOG_MESSAGE << "Play N";
                return min_winning_card;

            // Can't win
            } else if (hinted_card) {
                LOG_MESSAGE << "Play O";
                _did_hint = true;
                return hinted_card;

            } else if (worst_non_trump_card) {
                LOG_MESSAGE << "Play P";
                return worst_non_trump_card;

            // Can't win
            } else if (worst_trump_card) {
                LOG_MESSAGE << "Play Q";
                return worst_trump_card;

            } else {
                LOG_MESSAGE << "Play R";
                return cards.front();
            }

        }

    // Not all cards are on the table
    } else {

        // Try to win
        if (min_winning_card && !(we_are_winning && winning_player_won)) {
            LOG_MESSAGE << "Play V";
            return min_winning_card;

        // Can't win
        } else if (hinted_card) {
            LOG_MESSAGE << "Play W";
            _did_hint = true;
            return hinted_card;

        } else if (worst_non_trump_card) {
            LOG_MESSAGE << "Play X";
            return worst_non_trump_card;

        // Can't win
        } else if (worst_trump_card) {
            LOG_MESSAGE << "Play Y";
            return worst_trump_card;

        } else {
            LOG_MESSAGE << "Play Z";
            return cards.front();
        }
    }

    ASSERT(0);
    return nullptr;
}

//==============================================================================
//==============================================================================

void KaiserAI::finalize(Player p,
                        Player first_player,
                        const std::shared_ptr<KaiserObjectCard> played_cards[4],
                        const std::shared_ptr<KaiserBid> &top_bid)
{

    //
    // Track highest cards
    //

    for (DTshort i = 0; i < 4; ++i) {
        for (DTshort pp = 0; pp < 4; ++pp) {

            Suit s = played_cards[pp]->suit();

            if (played_cards[pp]->value() == _highest_card[s])
                _highest_card[s] = (Card) (_highest_card[s] - 1);
        }
    }

    //
    // Check for 3
    //

    for (DTshort pp = 0; pp < 4; ++pp) {
        if (played_cards[pp]->value() == CARD_3) {
            _did_play_3 = true;
            break;
        }
    }

    //
    // Update hints
    //

    if (!top_bid->is_no() && _hinted_suit == SUIT_UNDEFINED)
        return;

    // As per kevin - only your partner does the trick
    if (p != PLAYER_2)
        return;

    Player partner_p;

    if (p == PLAYER_0)  partner_p = PLAYER_2;
    if (p == PLAYER_1)  partner_p = PLAYER_3;
    if (p == PLAYER_2)  partner_p = PLAYER_0;
    if (p == PLAYER_3)  partner_p = PLAYER_1;

    if (played_cards[partner_p]->suit() != played_cards[first_player]->suit())
        _hinted_suit = played_cards[partner_p]->suit();

}

//==============================================================================
//==============================================================================

} // DT3

