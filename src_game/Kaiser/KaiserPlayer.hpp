#ifndef DT3_KAISERPLAYER
#define DT3_KAISERPLAYER
//==============================================================================
///	
///	File: KaiserPlayer.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/Types/Base/BaseClass.hpp"
#include <vector>

#include "Kaiser/KaiserObjectCard.hpp"

namespace DT3 {

//==============================================================================
/// Forward declarations
//==============================================================================

//==============================================================================
//==============================================================================

class KaiserPlayer: public BaseClass {    
    public:
        DEFINE_TYPE(KaiserPlayer,BaseClass)
		DEFINE_CREATE_AND_CLONE

                                                                KaiserPlayer            (void);
                                                                KaiserPlayer            (const KaiserPlayer &rhs);
        KaiserPlayer &                                          operator =				(const KaiserPlayer &rhs);
        virtual                                                 ~KaiserPlayer           (void);
    
    public:
        /// Add a card
        void                                                    add_card                (const std::shared_ptr<KaiserObjectCard> &card);

        /// Remove a card
        void                                                    remove_card             (const std::shared_ptr<KaiserObjectCard> &card);
    
        /// Return cards
        const std::vector<std::shared_ptr<KaiserObjectCard>> &  cards                   (void)  {   return _cards;                      }

        /// Return tricks
        struct Trick {
            std::shared_ptr<KaiserObjectCard>   _played_cards[4];
        };

        std::vector<Trick>&                                     tricks                  (void)  {   return _tricks;                     }

        /// Sort cards
        void                                                    sort                    (void);

        /// Swap cards
        void                                                    swap                    (   const std::shared_ptr<KaiserObjectCard> &old_card,
                                                                                            const std::shared_ptr<KaiserObjectCard> &new_card);

        /// Clear cards and tricks
        void                                                    clear                   (void)  {   _cards.clear(); _tricks.clear();    }
    
        /// Add trick
        void                                                    add_trick               (const std::shared_ptr<KaiserObjectCard> played_cards[4]);
    
        /// Num tricks
        DTshort                                                 number_of_tricks        (void)  {   return _tricks.size();              }
    
        /// Score tricks
        DTshort                                                 score                   (void);

        /// Has 5 hearts
        DTboolean                                               has_5_hearts            (void);

        /// Limit cards
        void                                                    limit                   (Suit led_suit);

        /// Clear Limist cards
        void                                                    clear_limits            (void);
    

    private:
        std::vector<std::shared_ptr<KaiserObjectCard>>  _cards;

        std::vector<Trick>                              _tricks;
    
};

//==============================================================================
//==============================================================================

} // DT3

#endif
