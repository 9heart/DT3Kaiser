#ifndef DT3_KAISERMAINMENU
#define DT3_KAISERMAINMENU
//==============================================================================
///	
///	File: 			KaiserMainMenu.hpp
///
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/Objects/GUIController.hpp"
#include "DT3Core/Types/FileBuffer/FilePath.hpp"
#include "DT3Core/Types/Node/Plug.hpp"
#include "DT3Core/Types/Network/URL.hpp"
#include "DT3InAppPurchases/InAppPurchasesProduct.hpp"
#include "DTPortalSDK/DTPortalLib/DTPortalSDK.hpp"
#include <map>

using namespace DTPortal;

//==============================================================================
/// Namespace
//==============================================================================

namespace DT3 {

//==============================================================================
/// Forward declarations
//==============================================================================

class GUIGridLayout;

//==============================================================================
/// Class
//==============================================================================

class KaiserMainMenu: public GUIController {
    public:
        DEFINE_TYPE(KaiserMainMenu,GUIController)
		DEFINE_CREATE_AND_CLONE
  		DEFINE_PLUG_NODE

											KaiserMainMenu			(void);	
    
											KaiserMainMenu			(const KaiserMainMenu &rhs);
        KaiserMainMenu &					operator =				(const KaiserMainMenu &rhs);	
    
        virtual								~KaiserMainMenu         (void);
    
        virtual void                        archive                 (const std::shared_ptr<Archive> &archive);
	
		/// Object was added to a world
		/// world world that object was added to
        virtual void                        add_to_world            (World *world);

		/// Object was removed from a world
        virtual void                        remove_from_world       (void);
    
		/// Description
		/// \param param description
		/// \return description
        void                                click_play              (void);

		/// Description
		/// \param param description
		/// \return description
        void                                click_high_scores       (void);

		/// Description
		/// \param param description
		/// \return description
        void                                click_store             (void);

		/// Description
		/// \param param description
		/// \return description
        void                                click_instructions      (void);

		/// Description
		/// \param param description
		/// \return description
        void                                click_credits           (void);
    

		/// Description
		/// \param param description
		/// \return description
        void                                click_back              (void);

		/// Description
		/// \param param description
		/// \return description
        void                                click_buy               (InAppPurchasesProduct product);

		/// Description
		/// \param param description
		/// \return description
        void                                click_restore           (void);

		/// Description
		/// \param param description
		/// \return description
        void                                click_ad                (URL url);

		/// Description
		/// \param param description
		/// \return description
        void                                click_ticker            (void);

		/// Description
		/// \param param description
		/// \return description
        void                                click_facebook          (void);

		/// Description
		/// \param param description
		/// \return description
        void                                click_twitter           (void);

		/// Description
		/// \param param description
		/// \return description
        void                                go_to_store             (void);

		/// Description
		/// \param param description
		/// \return description
		virtual void						tick					(const DTfloat dt);

    private:
        static void                         end_buy                         (std::string product, DTerr error, void *data);
        static void                         end_restore                     (std::string product, DTerr error, void *data);

        static std::shared_ptr<GUIObject>   end_refresh_high_scores_item    (const std::string &label, const std::shared_ptr<KaiserMainMenu> &menu);
        static void                         end_refresh_high_scores         (std::map<std::string,std::vector<DTPortal::HighScore> > &, DTPortal::DTPerror error, void *data);
        void                                begin_refresh_high_scores       (void);

        static void                         end_refresh_store               (std::vector<InAppPurchasesProduct> products, DTerr error, void *data);
        void                                begin_refresh_store             (void);
    
        static void                         strings_callback                (std::map<std::string,std::string> &, DTPerror error, void *data);

        void                                show_error                      (std::string error);
        void                                dismiss_error                   (void);

        enum State {
            STATE_MAIN_MENU,
            STATE_HIGH_SCORES,
            STATE_STORE
        } _state;
    
        void                                set_state               (State state, DTfloat t = 0.5F);

        //
        // Ticker
        //

        std::shared_ptr<GUIGridLayout>      _ticker_layout;
    
        std::string                         _ticker_text;
        std::string                         _ticker_url;
        std::shared_ptr<GUIObject>          _ticker;
        DTfloat                             _ticker_timer;

        //
        // Ads
        //
    
        std::shared_ptr<GUIGridLayout>      _ads_layout;
    

        //
        // Social Media
        //

#if DT3_OS != DT3_ANDROID
        std::shared_ptr<GUIGridLayout>      _social_media_layout;
#endif

        //
        // Back
        //
    
        std::shared_ptr<GUIGridLayout>      _back_layout;

        //
        // Main Menu
        //

        std::shared_ptr<GUIGridLayout>      _main_menu_layout;
    
        std::shared_ptr<GUIObject>          _play;
        std::shared_ptr<GUIObject>          _high_scores;
        std::shared_ptr<GUIObject>          _store;
        std::shared_ptr<GUIObject>          _instructions;

        //
        // High Scores
        //

        std::shared_ptr<GUIGridLayout>      _high_scores_layout;

        std::shared_ptr<GUIObject>          _high_scores_scroller;
        std::shared_ptr<GUIObject>          _high_scores_loading;

        //
        // Store
        //
    
        std::shared_ptr<GUIGridLayout>      _store_layout;

        std::shared_ptr<GUIObject>          _store_scroller;
        std::shared_ptr<GUIObject>          _store_loading;

        //
        // Error
        //

        std::shared_ptr<GUIGridLayout>      _error_layout;
    

        //
        // Current state
        //
    
        Plug<DTboolean>                     _is_main_menu;
        Plug<DTboolean>                     _is_high_scores;
        Plug<DTboolean>                     _is_store;


        // Animated loading indicator
        DTfloat                             _angle;
};

//==============================================================================
//==============================================================================

} // DT3

#endif
