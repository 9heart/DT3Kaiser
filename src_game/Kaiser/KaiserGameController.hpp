#ifndef DT3_KAISERGAMECONTROLLER
#define DT3_KAISERGAMECONTROLLER
//==============================================================================
///	
///	File: KaiserGameController.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================
//
// Dealing
//
// Kaiser is played by four people: two teams of two players each. Unlike many
// card games, only 32 cards are used out of a normal 52-card deck. The deck
// contains the cards from 8 to ace inclusively (8, 9, 10, jack, queen, king, ace)
// for each suit. The other four cards are the 7 of clubs, 7 of diamonds, 5 of
// hearts and 3 of spades. All 32 cards are dealt out: 8 to each player. The cards
// may be dealt in any order to any player at so long as each player ends up with
// 8 cards.
//
// Play
//
// In a clockwise manner, starting with the player to the dealer's left, each
// player may bid on the number of points that he believes he can make. The
// minimum bid is established before the game, with the most common value being
// 7.
// • Players must bid higher than the current bid or pass, with the exception
//   of the dealer who may take the bid at the current value. A bid is only for the
//   number of tricks and not which suit will be trump, with the exception of a
//   no-trump bid.
// • Bids range from the minimum bid to 12 with a no-trump bid being
//   greater than a trump bid (for example, 8 no-trump is larger than an 8 bid but
//   smaller than a 9 bid).
// • After a successful bid, the person who won the bid declares trump (unless it
//   was a no-trump bid) and plays any card they choose.
// • If no players bid then the dealer must make a "forced bid" for the set
//   minimum bid (although he can pick any suit or no-trump).
// • Players must follow suit if able (they cannot "trump in" if they have a
//   card in the suit that was led).
// • The player who played the highest card in that suit if no trump has been
//   played or the player who played the highest trump card takes the trick and
//   plays the next card of their choosing. That trick is worth one point towards
//   their score, unless it contains the 5 of hearts or 3 of spades. The trick that
//   contains the 5 of hearts is worth an extra 5 points (+6 net), while the trick
//   that contains the 3 of spades (the "three-spot") is worth 3 fewer points (−2
//   net).
// • Play continues until all cards have been played.
// 
// There are two special types of bids: no-trump and kaiser. A successful no-trump
// bid will mean that there are no trump cards. Simply, the highest card played
// (following suit) wins the trick. A kaiser bid is equivalent to a 12 no (no-trump)
// bid. To make 12 points, the bidder (not the team—note this is the only type of
// bid in the game of Kaiser where the member of the team taking the trick makes
// a difference) must take seven tricks, including the 5 of hearts, while forcing
// the opposing team to take the 3 of spades in the one remaining trick. In the
// very rare case of a kaiser bid, the bidding team immediately wins the game (or
// loses, in the event of an unsuccessful result).
// 
// Once all cards have been played, each team counts up the number of points they
// have made. If the bidding team made at least the amount they bid, they score
// the number of points they made (or twice that amount for a no-trump bid). If
// they did not, they lose the amount they bid (or twice the amount they bid for
// a no-trump bid). The opposing team gains the amount of tricks they have made
// and adds that to their score, unless they are at "bid-out". The bid-out point
// is where a team must bid in order to count an increase in points. This point
// is usually 45 to 47 and is calculated by subtracting the agreed-upon minimum
// bid from 52. The game ends when a team bids and gets to 52 points, at which
// point they are declared the winner. One variation increases the number of points
// to win from 52 to 62 if a no-trump bid is made. This also increases the bid-out
// point by 10 points.
//
//==============================================================================
//==============================================================================

#include "DT3Core/Objects/GameController.hpp"
#include "DT3Core/Types/Math/Matrix4.hpp"
#include "DT3Core/Types/GUI/GUIGridLayout.hpp"
#include "DT3Core/Types/Utility/Coroutine.hpp"
#include <list>

#include "Kaiser/KaiserCommon.hpp"
#include "Kaiser/KaiserAI.hpp"
#include "Kaiser/KaiserBid.hpp"
#include "Kaiser/KaiserPlayer.hpp"
#include "Kaiser/KaiserInteraction.hpp"

namespace DT3 {

//==============================================================================
/// Forward declarations
//==============================================================================

class KaiserObjectCard;
class GUIController;

//==============================================================================
/// The base game controller. The Game controller manages the game rules and
/// state and also manages the players.
//==============================================================================

class KaiserGameController: public GameController {    
    public:
        DEFINE_TYPE(KaiserGameController,GameController)
		DEFINE_CREATE_AND_CLONE

										KaiserGameController    (void);
										KaiserGameController    (const KaiserGameController &rhs);
        KaiserGameController &			operator =				(const KaiserGameController &rhs);
        virtual							~KaiserGameController   (void);
    
        virtual void					archive                 (const std::shared_ptr<Archive> &archive);
        virtual void                    archive_done            (const std::shared_ptr<Archive> &archive);

	public:
		/// Called to initialize the object
		virtual void                    initialize				(void);
    
        /// Callback called when the object is getting a tick
        void                            run_co                  (Coroutine<KaiserGameController> *co) __attribute__ ((noreturn));

		/// Callback called when the object is getting a tick
		/// \param dt delta time
		virtual void                    tick                    (const DTfloat dt);

		/// Object was added to a world
		/// world world that object was added to
        virtual void                    add_to_world            (World *world);

		/// Object was removed from a world
        virtual void                    remove_from_world       (void);

    private:
        Coroutine<KaiserGameController> _co;

        //
        // Game State
        //
    
        GameState                       _state;
    
        KaiserPlayer                        _players[4];
        KaiserAI                            _ai[4];
        std::shared_ptr<KaiserObjectCard>   _played_cards[4];
        
        Player                          _dealer;
        Player                          _first_player_turn;
        Player                          _player_turn;

        std::vector<std::shared_ptr<KaiserObjectCard>>  _passed_cards;
    
        Player                          _highest_bidder;
        std::shared_ptr<KaiserBid>      _highest_bidder_bid;
    
        DTshort                         _minimum_bid;
    
        std::shared_ptr<KaiserBid>      _bid;
    
        DTboolean                       _human_player;
        DTboolean                       _submitted;
    
        DTshort                         _us_score;
        DTshort                         _them_score;
        DTshort                         _us_score_round;
        DTshort                         _them_score_round;

        DTboolean                       _options_set;
        DTboolean                       _option_no_trump_bidout;
        DTboolean                       _option_pass_cards;
        DTboolean                       _option_game_over_52;
//        DTboolean                       _option_steal_5_is_win;

        DTboolean                       _pause_sound_on;

        DTboolean                       _dismissed_nag;

        void                            animate_bid             (Player p, const std::shared_ptr<KaiserBid> &bid);
        void                            animate_trump           (Player p, const std::shared_ptr<KaiserBid> &bid);
        void                            animate_player_turn     (Player p);
        void                            animate_dealer_turn     (Player p);
        void                            animate_player_message  (Player p, const std::string &msg, DTfloat delay = 2.0F);
        void                            animate_hide_player_message(Player p);

        void                            animate_update_score    (void);

        void                            animate_win             (void);
        void                            animate_lose            (void);
        void                            animate_tie             (void);
    
        void                            random_fade_in          (const std::shared_ptr<GUILayout> &layout);
        void                            random_fade_out         (const std::shared_ptr<GUILayout> &layout);


        std::shared_ptr<KaiserObjectCard>   get_card            (Card c, Suit s);

        //
        // Card positions
        //

        std::vector<std::shared_ptr<KaiserObjectCard>>  _cards;
        std::vector<std::shared_ptr<KaiserObjectCard>>  _deck;
    
    
        //
        // UI
        //
    
        std::shared_ptr<GUIGridLayout>  _gui_layout_master;
    
        void                            sync_options                (void);
        void                            show_options_min_bidding    (void);
        void                            switch_options_min_bid      (DTshort bid);
        void                            switch_options              (void);
        void                            accept_options_min_bid      (void);

        void                            switch_options_no_trump_bidout  (void);
        void                            switch_options_pass_cards       (void);
        void                            switch_options_game_over_52     (void);
//        void                            switch_options_steal_5_is_win   (void);

        void                            switch_pause_sound      (void);

        void                            update_bidding          (void);
        void                            show_bidding            (void);
        void                            switch_bid              (DTshort bid);
        void                            pass_bid                (void);
        void                            accept_bid              (void);
    
        void                            show_trump              (DTboolean force);
        void                            switch_trump            (Suit trump);
        void                            accept_trump            (void);

        void                            go_to_store_nag         (void);
        void                            show_nag                (void);
        void                            dismiss_nag             (void);

        void                            show_pause              (void);
    
        void                            show_high_score         (void);
        void                            click_key               (std::string k);

        void                            play_card               (std::shared_ptr<KaiserObjectCard> card);
        void                            pass_card               (std::shared_ptr<KaiserObjectCard> card);
        void                            trick_card              (std::shared_ptr<KaiserObjectCard> card, DTboolean showing);

        void                            show_end_game           (const std::string &msg);
        void                            restart                 (void);
        void                            resume                  (void);
        void                            quit                    (void);
        void                            submit                  (void);
        void                            skip                    (void);

        void                            click_facebook          (void);
        void                            click_twitter           (void);

        //
        // Sound
        //
    
        void                            play_sound_shuffle      (void);
        void                            play_sound_deal         (void);
        void                            play_sound_click        (void);
        void                            play_sound_bubble       (void);


        std::shared_ptr<GUIObject>      make_key                (const std::string &key, DTfloat width, DTfloat height);
        std::shared_ptr<GUIObject>      make_key_space          (DTfloat width, DTfloat height);

        //
        // UI widgets
        //

#if DT3_OS != DT3_ANDROID
        // Social Media
        std::shared_ptr<GUIGridLayout>  _social_media_layout;
#endif

        // Interactions
        std::shared_ptr<GUIObject>          _interactions;
        std::shared_ptr<KaiserInteraction>  _interactions_component;

        // Pause
        std::shared_ptr<GUIGridLayout>  _gui_layout_pause;

        // Scoreboard
        std::shared_ptr<GUIGridLayout>  _gui_layout_scoreboard;
        std::shared_ptr<GUIObject>      _us_widget;
        std::shared_ptr<GUIObject>      _them_widget;
        std::shared_ptr<GUIObject>      _us_widget_title;
        std::shared_ptr<GUIObject>      _them_widget_title;

    
        // Turn indicator
        std::shared_ptr<GUIGridLayout>  _gui_layout_turn;
        std::shared_ptr<GUIObject>      _turn_player_0_widget;
        std::shared_ptr<GUIObject>      _turn_player_1_widget;
        std::shared_ptr<GUIObject>      _turn_player_2_widget;
        std::shared_ptr<GUIObject>      _turn_player_3_widget;

        std::shared_ptr<GUIObject>      _dealer_player_0_widget;
        std::shared_ptr<GUIObject>      _dealer_player_1_widget;
        std::shared_ptr<GUIObject>      _dealer_player_2_widget;
        std::shared_ptr<GUIObject>      _dealer_player_3_widget;

        // Player messages
        std::shared_ptr<GUIGridLayout>  _gui_layout_messages;
        std::shared_ptr<GUIObject>      _message_player_0_widget;
        std::shared_ptr<GUIObject>      _message_player_1_widget;
        std::shared_ptr<GUIObject>      _message_player_2_widget;
        std::shared_ptr<GUIObject>      _message_player_3_widget;
    
        // Options
        std::shared_ptr<GUIGridLayout>  _gui_layout_options_min_bidding;
        std::shared_ptr<GUIObject>      _options_min_bid_5_widget;
        std::shared_ptr<GUIObject>      _options_min_bid_6_widget;
        std::shared_ptr<GUIObject>      _options_min_bid_7_widget;
        std::shared_ptr<GUIObject>      _options_min_bid_8_widget;

        std::shared_ptr<GUIObject>      _options_no_trump_bidout_widget;
        std::shared_ptr<GUIObject>      _options_pass_cards_widget;
        std::shared_ptr<GUIObject>      _options_game_over_52_widget;
//        std::shared_ptr<GUIObject>      _options_steal_5_to_win_widget;

        std::shared_ptr<GUIObject>      _pause_sound;

        // Bidding
        std::shared_ptr<GUIGridLayout>  _gui_layout_bidding;
        std::shared_ptr<GUIObject>      _bid_5_widget;
        std::shared_ptr<GUIObject>      _bid_6_widget;
        std::shared_ptr<GUIObject>      _bid_7_widget;
        std::shared_ptr<GUIObject>      _bid_8_widget;
        std::shared_ptr<GUIObject>      _bid_9_widget;
        std::shared_ptr<GUIObject>      _bid_10_widget;
        std::shared_ptr<GUIObject>      _bid_11_widget;
        std::shared_ptr<GUIObject>      _bid_12_widget;
        std::shared_ptr<GUIObject>      _bid_no_widget;
        std::shared_ptr<GUIObject>      _bid_bid_widget;
        std::shared_ptr<GUIObject>      _bid_pass_widget;
    
        // Declare Trump
        std::shared_ptr<GUIGridLayout>  _gui_layout_declare_trump;
        std::shared_ptr<GUIObject>      _trump_club_widget;
        std::shared_ptr<GUIObject>      _trump_diamond_widget;
        std::shared_ptr<GUIObject>      _trump_heart_widget;
        std::shared_ptr<GUIObject>      _trump_spade_widget;
        std::shared_ptr<GUIObject>      _trump_no_widget;
        std::shared_ptr<GUIObject>      _trump_go_widget;
    
        // End of Game
        std::shared_ptr<GUIGridLayout>  _gui_end_of_game;
        std::shared_ptr<GUIObject>      _gui_end_of_game_score;
        std::shared_ptr<GUIObject>      _gui_end_of_game_message;

        // Full Version
        std::shared_ptr<GUIGridLayout>  _get_full_version_layout;


        // Pause
        std::shared_ptr<GUIGridLayout>  _gui_pause_button;
        std::shared_ptr<GUIGridLayout>  _gui_pause;

        // High score
        std::shared_ptr<GUIGridLayout>  _enter_high_score_layout;
        std::shared_ptr<GUIGridLayout>  _enter_high_score_rows;
        std::shared_ptr<GUIGridLayout>  _row1;
        std::shared_ptr<GUIGridLayout>  _row2;
        std::shared_ptr<GUIGridLayout>  _row3;
        std::shared_ptr<GUIGridLayout>  _row4;
        std::shared_ptr<GUIGridLayout>  _row5;
        std::shared_ptr<GUIGridLayout>  _row_buttons;
        std::shared_ptr<GUIObject>      _text_box;

        // GUI Controler
        std::shared_ptr<GUIController>  _gui_controller;
        
};

//==============================================================================
//==============================================================================

} // DT3

#endif
