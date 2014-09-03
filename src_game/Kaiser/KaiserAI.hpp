#ifndef DT3_KAISERAI
#define DT3_KAISERAI
//==============================================================================
///	
///	File: KaiserAI.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/Types/Base/BaseClass.hpp"
#include "Kaiser/KaiserCommon.hpp"
#include "Kaiser/KaiserBid.hpp"
#include <vector>

namespace DT3 {

//==============================================================================
/// Forward declarations
//==============================================================================

class KaiserObjectCard;

//==============================================================================
/// Base object for the different placeable types of objects in the engine.
//==============================================================================

class KaiserAI: public BaseClass {
    public:
        DEFINE_TYPE(KaiserAI,BaseClass)
		DEFINE_CREATE_AND_CLONE
    
                                    KaiserAI                (void);
                                    KaiserAI                (const KaiserAI &rhs);
        KaiserAI &                  operator =				(const KaiserAI &rhs);
        virtual                     ~KaiserAI               (void);
    
	public:
    
		/// Called to initialize the class
		static void                 initialize_static       (void);

		/// Called to uninitialize the class
		static void                 uninitialize_static     (void);


        /// Begin
        void                                begin   (   Player p);

        /// Get Pass cards
        void                                pass    (   Player p,
                                                        const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                                                        std::shared_ptr<KaiserObjectCard> &c0,
                                                        std::shared_ptr<KaiserObjectCard> &c1   );

		/// Get Bid
        std::shared_ptr<KaiserBid>          bid     (   Player p,
                                                        Player dealer,
                                                        const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                                                        DTshort minimum_bid,
                                                        DTboolean forced_bid,
                                                        const std::shared_ptr<KaiserBid> &top_bid);

		/// Get Trump
        void                                trump   (   Player p,
                                                        const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                                                        DTboolean forced_bid,
                                                        const std::shared_ptr<KaiserBid> &top_bid);
    
        /// Play
        std::shared_ptr<KaiserObjectCard>   play    (   Player p,
                                                        Player first_player,
                                                        const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                                                        const std::shared_ptr<KaiserObjectCard> played_cards[4],
                                                        const std::shared_ptr<KaiserBid> &top_bid);

        /// Finalize
        void                                finalize(   Player p,
                                                        Player first_player,
                                                        const std::shared_ptr<KaiserObjectCard> played_cards[4],
                                                        const std::shared_ptr<KaiserBid> &top_bid);
    
    private:
        Suit                                        trump_suit  (   Player p,
                                                                    const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards,
                                                                    const std::shared_ptr<KaiserBid> &top_bid);

        static std::shared_ptr<KaiserObjectCard>    find_card   (DTshort value, Suit suit, const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards);
        DTboolean                                   has_card    (DTshort value, Suit suit, const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards);

        std::string                                 print_card  (const std::shared_ptr<KaiserObjectCard> &card);


        DTfloat                             weighted_sum_for_suit   (Suit s, const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards);
        DTfloat                             num_for_suit            (Suit s, const std::vector<std::shared_ptr<KaiserObjectCard>> &player_cards);

        DTboolean                           _did_hint;
        Suit                                _hinted_suit;

        Suit                                _run_suit;
        DTboolean                           _did_play_3;

        Card                                _highest_card[4];
};

//==============================================================================
//==============================================================================

} // DT3

#endif
