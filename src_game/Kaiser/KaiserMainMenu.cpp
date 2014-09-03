//==============================================================================
///	
///	File: 			KaiserMainMenu.cpp
///
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserMainMenu.hpp"
#include "DT3Core/System/Factory.hpp"
#include "DT3Core/System/System.hpp"
#include "DT3Core/System/Application.hpp"
#include "DT3Core/System/Globals.hpp"
#include "DT3Core/Objects/GUIObject.hpp"
#include "DT3Core/World/World.hpp"
#include "DT3Core/Types/FileBuffer/Archive.hpp"
#include "DT3Core/Types/Math/Rectangle.hpp"
#include "DT3Core/Types/Math/MoreMath.hpp"
#include "DT3Core/Types/GUI/GUIGridLayout.hpp"
#include "DT3Core/Types/GUI/GUIAnimKey.hpp"
#include "DT3Core/Types/GUI/GUILayoutPolicy.hpp"
#include "DT3Core/Types/Utility/ConsoleStream.hpp"
#include "DT3Core/Types/Utility/MoreStrings.hpp"
#include "DT3Core/Types/Network/URL.hpp"
#include "DT3Core/Types/Memory/RefCounter.hpp"
#include "DT3Core/Components/ComponentGUIDrawText.hpp"
#include "DT3Core/Components/ComponentGUIScrollerLayout.hpp"
#include "DT3Core/Components/ComponentGUIDrawIcon.hpp"
#include "DT3Core/Components/ComponentGUIDrawButton.hpp"
#include "DT3Core/Components/ComponentGUIButton.hpp"

#include "DT3InAppPurchases/InAppPurchases.hpp"
#include <vector>

#include DT3_HAL_INCLUDE_PATH

#include "Kaiser/KaiserDonkeyAds.hpp"

#ifndef DT3_EDITOR
    extern void show_twitter    (const std::string &msg);
    extern void show_facebook   (const std::string &msg);

    extern void show_ads        (void);
    extern void hide_ads        (void);
#else
    inline void show_twitter    (const std::string &msg)    {}
    inline void show_facebook   (const std::string &msg)    {}

    inline void show_ads        (void)                      {}
    inline void hide_ads        (void)                      {}
#endif

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
/// Register with object factory
//==============================================================================

IMPLEMENT_FACTORY_CREATION_PLACEABLE(KaiserMainMenu,"Kaiser","EdPlaceableObjectAdapter")
IMPLEMENT_PLUG_NODE(KaiserMainMenu)

IMPLEMENT_PLUG_INFO_INDEX(_is_main_menu)
IMPLEMENT_PLUG_INFO_INDEX(_is_high_scores)
IMPLEMENT_PLUG_INFO_INDEX(_is_store)

//==============================================================================
//==============================================================================

BEGIN_IMPLEMENT_PLUGS(KaiserMainMenu)

	PLUG_INIT(_is_main_menu,"Is_Main_Menu")
		.set_output(true);

	PLUG_INIT(_is_high_scores,"Is_High_Scores")
		.set_output(true);

	PLUG_INIT(_is_store,"Is_Store")
		.set_output(true);

END_IMPLEMENT_PLUGS

//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserMainMenu::KaiserMainMenu (void)
    :   _state                  (STATE_MAIN_MENU),
		_is_main_menu           (PLUG_INFO_INDEX(_is_main_menu), true),
		_is_high_scores         (PLUG_INFO_INDEX(_is_high_scores), false),
		_is_store               (PLUG_INFO_INDEX(_is_store), false),
        _angle                  (0.0F),
        _ticker_timer           (0.0F)
{
	set_streamable(true);
}
		
KaiserMainMenu::KaiserMainMenu (const KaiserMainMenu &rhs)
    :	GUIController           (rhs),
        _state                  (rhs._state),
        _is_main_menu           (rhs._is_main_menu),
        _is_high_scores         (rhs._is_high_scores),
        _is_store               (rhs._is_store),
        _angle                  (rhs._angle)
{

}

KaiserMainMenu & KaiserMainMenu::operator = (const KaiserMainMenu &rhs)
{
    // Make sure we are not assigning the class to itself
    if (&rhs != this) {
		GUIController::operator = (rhs);
        
        _state = rhs._state;
        
        _is_main_menu = rhs._is_main_menu;
        _is_high_scores = rhs._is_high_scores;
        _is_store = rhs._is_store;
        _angle = rhs._angle;

	}
    return (*this);
}
			
KaiserMainMenu::~KaiserMainMenu (void)
{	

}

//==============================================================================
//==============================================================================

void KaiserMainMenu::archive (const std::shared_ptr<Archive> &archive)
{
    GUIController::archive(archive);

    archive->push_domain (class_id());
    archive->pop_domain ();
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::set_state (State state, DTfloat t)
{
    Rectangle left(-1.0F,0.0F,0.0F,1.0F);
    Rectangle center(0.0F,1.0F,0.0F,1.0F);
    Rectangle up(0.0F,1.0F,1.0F,2.0F);
    Rectangle right(1.0F,2.0F,0.0F,1.0F);


    switch (state) {
        case STATE_MAIN_MENU:
            _main_menu_layout->animate(center, t, true);
            if (_ads_layout)
                _ads_layout->animate(center, t, true);
            _high_scores_layout->animate(right, t, true);
            _store_layout->animate(right, t, true);
            _ticker_layout->animate(center, t, true);
            _back_layout->animate(up, t, true);
            
            _is_main_menu = true;
            _is_high_scores = false;
            _is_store = false;

            break;
            
        case STATE_HIGH_SCORES:
            _main_menu_layout->animate(left, t, true);
            if (_ads_layout)
                _ads_layout->animate(left, t, true);
            _high_scores_layout->animate(center, t, true);
            _store_layout->animate(right, t, true);
            _ticker_layout->animate(up, t, true);
            _back_layout->animate(center, t, true);

            _is_main_menu = false;
            _is_high_scores = true;
            _is_store = false;

            break;
            
        case STATE_STORE:
            _main_menu_layout->animate(left, t, true);
            if (_ads_layout)
                _ads_layout->animate(left, t, true);
            _high_scores_layout->animate(right, t, true);
            _store_layout->animate(center, t, true);
            _ticker_layout->animate(up, t, true);
            _back_layout->animate(center, t, true);

            _is_main_menu = false;
            _is_high_scores = false;
            _is_store = true;

            break;
            
    }
    
    _state = state;
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::click_play (void)
{
    System::application()->transition_to_world(FilePath("{kaiser.lvl}"), "TransitionFadeOutIn", 1.0F, NULL, NULL);

#ifdef DT3_DEBUG
    Globals::set_global("APP_FULL_VERSION","1",Globals::VOLATILE);
#endif

    if (!MoreStrings::cast_from_string<DTboolean>(Globals::global("APP_FULL_VERSION"))) {
        show_ads();
    }
}

void KaiserMainMenu::click_high_scores (void)
{
    set_state(STATE_HIGH_SCORES);
    
    begin_refresh_high_scores();
}

void KaiserMainMenu::click_store (void)
{
    set_state(STATE_STORE);
    
    begin_refresh_store();
}

void KaiserMainMenu::click_instructions (void)
{
    HAL::launch_browser(URL("http://kaiser.smellslikedonkey.com/instructions.html"));
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::click_back (void)
{
    set_state(STATE_MAIN_MENU);
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::end_buy(std::string product, DTerr error, void *data)
{
    LOG_MESSAGE << "KaiserMainMenu::end_buy called for " << product;
    
    if (data) {
        RefCounter<std::shared_ptr<KaiserMainMenu>> *rc = (RefCounter<std::shared_ptr<KaiserMainMenu>>*) data;
        std::shared_ptr<KaiserMainMenu> menu = rc->get();
        ASSERT(menu);
        
        if (error != DT3_ERR_NONE) {
            menu->show_error("{TXT_APP_STORE_ERROR}");

        } else {
            Globals::set_global("APP_PURCHASE_" + product, "1", Globals::PERSISTENT_OBFUSCATED);
            
            // Set products
            if (product == "KAISERFULLAPP" || product == "kaiserfullapp")
                Globals::set_global("APP_FULL_VERSION", "1", Globals::PERSISTENT_OBFUSCATED);
            
            // Save the purchase
            Globals::save();

            menu->click_back();
        }

        // Animate indicator
        menu->_store_loading->add_anim_key()
            .set_duration(0.5F)
            .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
      
        // Release ref counter
        rc->release();
    }
}

void KaiserMainMenu::click_buy(InAppPurchasesProduct product)
{
    auto rc = new RefCounter<std::shared_ptr<KaiserMainMenu>>(checked_cast<KaiserMainMenu>(shared_from_this()));
    InAppPurchases::make_purchase (product, &KaiserMainMenu::end_buy, rc);
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::end_restore(std::string product, DTerr error, void *data)
{
    end_buy(product, error, data);
}

void KaiserMainMenu::click_restore(void)
{
    auto rc = new RefCounter<std::shared_ptr<KaiserMainMenu>>(checked_cast<KaiserMainMenu>(shared_from_this()));
    InAppPurchases::restore_products (&KaiserMainMenu::end_restore, rc);
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::click_ad (URL url)
{
    HAL::launch_browser(url);
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::click_ticker (void)
{
    if (_ticker_url.size() > 0) {
        HAL::launch_browser(URL(_ticker_url));
    }
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::click_facebook (void)
{
    show_facebook (Globals::global("TXT_FACEBOOK_MSG"));
}

void KaiserMainMenu::click_twitter (void)
{
    show_twitter (Globals::global("TXT_TWITTER_MSG"));
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::go_to_store (void)
{
    click_store();
}

//==============================================================================
//==============================================================================

std::shared_ptr<GUIObject> KaiserMainMenu::end_refresh_high_scores_item (const std::string &label, const std::shared_ptr<KaiserMainMenu> &menu)
{
    std::shared_ptr<GUIObject> widget = GUIObject::create();
    menu->world()->add_node(widget);
    menu->add_child(widget);
    
    widget->set_label("{FMT_SCORE_FORMATTING}" + label);
    
    std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
    widget->add_component(widget_drawing);
    widget_drawing->set_center_vertically(true);
    widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
    widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    
    return widget;
}

void KaiserMainMenu::end_refresh_high_scores(std::map<std::string,std::vector<DTPortal::HighScore> > &scores, DTPortal::DTPerror error, void *data)
{
    if (data) {
        RefCounter<std::shared_ptr<KaiserMainMenu>> *rc = (RefCounter<std::shared_ptr<KaiserMainMenu>>*) data;
        std::shared_ptr<KaiserMainMenu> menu = rc->get();
        ASSERT(menu);

        // Clear all children from the item
        menu->_high_scores_scroller->remove_children();
        

        FOR_EACH(i, scores) {
        
            if (i->first != "Kaiser")
                continue;
        
            std::vector<DTPortal::HighScore> &s = i->second;
            
            // New layout
            std::shared_ptr<GUIGridLayout> layout = GUIGridLayout::create();
            
            layout->set_rows_and_columns( (DTint) s.size(), 3);
            layout->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
            layout->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 4.0F));
            layout->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 4.0F));

            std::shared_ptr<ComponentGUIScrollerLayout> grid = checked_cast<ComponentGUIScrollerLayout>(menu->_high_scores_scroller->component_by_type(ComponentBase::COMPONENT_TOUCH));
            grid->set_layout(layout);
            grid->set_content_height(s.size() * 0.05F);


            for (DTint j = 0; j < s.size(); ++j) {
                DTint r = s.size() - j - 1;
            
                layout->set_row_policy(j, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
            
                LOG_MESSAGE << s[j].name.c_str() << "  " << s[j].score;
                
                // Number
                std::shared_ptr<GUIObject> number = end_refresh_high_scores_item(MoreStrings::cast_to_string(j+1) + ".", menu);
                layout->add_item(r, 0, number);
                menu->_high_scores_scroller->add_child(number);
                
                // Name
                std::shared_ptr<GUIObject> name;
                if (s[j].censored)  name = end_refresh_high_scores_item("?????", menu);
                else                name = end_refresh_high_scores_item(s[j].name.c_str(), menu);
                layout->add_item(r, 1, name);
                menu->_high_scores_scroller->add_child(name);
                
                // Score
                DTushort wins = (s[j].score >> 16) & 0xFFFF;
                DTushort losses = (s[j].score) & 0xFFFF;

                std::string ss = MoreStrings::cast_to_string(wins) + " wins/" + MoreStrings::cast_to_string(losses) + " losses";

                std::shared_ptr<GUIObject> score;
                score = end_refresh_high_scores_item(ss, menu);
                layout->add_item(r, 2, score);
                menu->_high_scores_scroller->add_child(score);
            }
            
            // Arrange Items
            grid->arrange_items(0.0F);
            grid->scroll_to_top();

        }
        
     
        // Animate indicator
        menu->_high_scores_loading->add_anim_key()
            .set_duration(0.5F)
            .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Release ref counter
        rc->release();
    }

}

void KaiserMainMenu::begin_refresh_high_scores (void)
{
    // Clear all children from the item
    _high_scores_scroller->remove_children();
    
    // Animate indicator
    _high_scores_loading->add_anim_key()
        .set_duration(0.5F)
        .set_color(Color4f(1.0F,1.0F,1.0F,1.0F));

    // Hack for C callbacks
    auto rc = new RefCounter<std::shared_ptr<KaiserMainMenu>>(checked_cast<KaiserMainMenu>(shared_from_this()));
    DTPortal::high_scores(&KaiserMainMenu::end_refresh_high_scores, rc);
    
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::end_refresh_store(std::vector<InAppPurchasesProduct> products, DTerr error, void *data)
{
    LOG_MESSAGE << "end_refresh_store ";

    if (data) {
        RefCounter<std::shared_ptr<KaiserMainMenu>> *rc = reinterpret_cast<RefCounter<std::shared_ptr<KaiserMainMenu>>*>(data);

        std::shared_ptr<KaiserMainMenu> menu = rc->get();
        ASSERT(menu);

        DTfloat screen_width = System::renderer()->screen_width();
        DTfloat screen_height = System::renderer()->screen_height();
        DTboolean phone_mode = screen_height > screen_width;

        menu->_store_scroller->remove_children();

        const DTfloat ITEM_HEIGHT = phone_mode ? (350.0F + 10.0F + 10.0F)/screen_height : (128.0F + 10.0F + 10.0F + 40.0F + 10.0F)/screen_height;

        // Scroller
        std::shared_ptr<ComponentGUIScrollerLayout> scroller_layout = checked_cast<ComponentGUIScrollerLayout>(menu->_store_scroller->component_by_type(ComponentBase::COMPONENT_TOUCH));

        if (error == DT3_ERR_NONE) {
            scroller_layout->set_content_height(products.size() * ITEM_HEIGHT);

            // Build the layout for the scroller
            std::shared_ptr<GUIGridLayout> layout = GUIGridLayout::create();
            scroller_layout->set_layout(layout);

            layout->set_rows_and_columns( (DTint) products.size(), 1);
            
            for (DTint i = 0; i < products.size(); ++i) {
            
                std::shared_ptr<GUIGridLayout> icon_layout = GUIGridLayout::create();
                std::shared_ptr<GUIGridLayout> item_layout = GUIGridLayout::create();
                std::shared_ptr<GUIGridLayout> title_descrption_layout = GUIGridLayout::create();

                icon_layout->set_rows_and_columns(4, 1);
                icon_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

                if (phone_mode) {
                    icon_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 128.0F/screen_height));
                } else {
                    icon_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 40.0F/screen_height));
                }
                icon_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
                icon_layout->set_row_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 128.0F/screen_height));

                title_descrption_layout->set_rows_and_columns(3, 1);
                title_descrption_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
                title_descrption_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));

                if (phone_mode) {
                    title_descrption_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_height));
                } else {
                    title_descrption_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 60.0F/screen_height));
                }

                item_layout->set_rows_and_columns(3, 5);
                item_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
                item_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
                item_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
                item_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
                item_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 128.0F/screen_width));
                item_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
                item_layout->set_column_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
                item_layout->set_column_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
                
                item_layout->add_item(1, 3, title_descrption_layout);
                item_layout->add_item(1, 1, icon_layout);
                
                layout->add_item(i, 0, item_layout);
                
                // Background
                {
                    std::shared_ptr<GUIObject> widget = GUIObject::create();
                    menu->world()->add_node_unique_name(widget);
                    menu->_store_scroller->add_child(widget);
                    
                    layout->add_item(i, 0, widget);

                    widget->set_no_focus(true);
                    widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));

                    // Drawing
                    std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
                    widget->add_component(widget_drawing);
                    
                    widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg2.mat}")));
                    widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
                    widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
                    widget_drawing->set_corner_height(0.025F);
                    widget_drawing->set_corner_width(0.025F);
                }

                // Image
                {
                    std::shared_ptr<GUIObject> widget = GUIObject::create();
                    menu->world()->add_node_unique_name(widget);
                    menu->_store_scroller->add_child(widget);
                    
                    icon_layout->add_item(3, 0, widget);
                    
                    widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
                    
                    // Drawing
                    std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
                    widget->add_component(widget_drawing);
                    
                    widget_drawing->set_material(MaterialResource::import_resource(FilePath("{iap_" + products[i].product_identifier() + ".mat}")));
                    widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
                }
                
                // Store Title
                {
                    std::shared_ptr<GUIObject> widget = GUIObject::create();
                    menu->world()->add_node_unique_name(widget);
                    menu->_store_scroller->add_child(widget);
                    
                    title_descrption_layout->add_item(2, 0, widget);
                    
                    widget->set_label("{FMT_STORE_TITLE}" + products[i].title());
                    widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
                    
                    // Drawing
                    std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
                    widget->add_component(widget_drawing);
                    
                    widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
                    widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
                    widget_drawing->set_center_vertically(true);
                }

                // Description
                {
                    std::shared_ptr<GUIObject> widget = GUIObject::create();
                    menu->world()->add_node_unique_name(widget);
                    menu->_store_scroller->add_child(widget);
                    
                    title_descrption_layout->add_item(0, 0, widget);
                    
                    widget->set_label("{FMT_STORE_DESCRP}" + products[i].description());
                    widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
                    
                    // Drawing
                    std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
                    widget->add_component(widget_drawing);
                    
                    widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
                    widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
                    widget_drawing->set_center_vertically(true);
                }

                // Price
                {
                    std::shared_ptr<GUIObject> widget = GUIObject::create();
                    menu->world()->add_node_unique_name(widget);
                    menu->_store_scroller->add_child(widget);
                    
                    icon_layout->add_item(1, 0, widget);

                    widget->set_label("{FMT_STORE_PRICE}" + products[i].price());
                    widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
                    widget->set_no_focus(false);
                    
                    // Interaction 
                    std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
                    widget->add_component(widget_interaction);
                    widget_interaction->set_button_pressed_latent_call(make_latent_call(menu.get(), &type::click_buy, products[i]));

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

//                // Restore
//                {
//                    std::shared_ptr<GUIObject> widget = GUIObject::create();
//                    menu->world()->add_node_unique_name(widget);
//                    menu->_store_scroller->add_child(widget);
//                    
//                    button_layout->add_item(1, 0, widget);
//                    
//                    widget->set_label("{TXT_STORE_RESTORE}");
//                    widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
//                    widget->set_no_focus(false);
//                    
//                    // Interaction 
//                    std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
//                    widget->add_component(widget_interaction);
//                    widget_interaction->set_button_pressed_latent_call(make_latent_call(menu.get(), &type::click_restore, products[i]));
//
//                    // Drawing
//                    std::shared_ptr<ComponentGUIDrawButton> widget_drawing = ComponentGUIDrawButton::create();
//                    widget->add_component(widget_drawing);
//                    
//                    widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_button.mat}")));
//                    widget_drawing->set_disabled_material(MaterialResource::import_resource(FilePath("{ui_button_disabled.mat}")));
//                    widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
//                    widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
//                    widget_drawing->set_corner_height(0.025F);
//                    widget_drawing->set_corner_width(0.025F);
//                    
//                }

            }
            
        // Error
        } else {
            scroller_layout->set_content_height(ITEM_HEIGHT);

            // Build the layout for the scroller
            std::shared_ptr<GUIGridLayout> layout = GUIGridLayout::create();
            scroller_layout->set_layout(layout);
            
            layout->set_rows_and_columns(1, 1);

            // High Scores Title
            {
                std::shared_ptr<GUIObject> widget = GUIObject::create();
                menu->world()->add_node_unique_name(widget);
                menu->_store_scroller->add_child(widget);
                
                layout->add_item(0, 0, widget);
                
                widget->set_label("{TXT_APP_STORE_ERROR}");
                widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
                
                // Drawing
                std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
                widget->add_component(widget_drawing);
                
                widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
                widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
                widget_drawing->set_center_vertically(true);
            }
        }
        
        // Arrange all the items
        scroller_layout->arrange_items();
        scroller_layout->scroll_to_top();
      
        // Animate indicator
        menu->_store_loading->add_anim_key()
            .set_duration(0.5F)
            .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

        // Release ref counter
        rc->release();

    }
    
}

//void KaiserMainMenu::store_success_cb(std::string product_id)
//{
//    LOG_MESSAGE << "store_success_cb " << product_id;
//    
//    Globals::set_global("APP_PURCHASE_" + product_id, "1", Globals::PERSISTENT_OBFUSCATED);
//    
//    // Set products
//    if (product_id == "KAISERFULLAPP")
//        Globals::set_global("APP_FULL_VERSION", "1", Globals::PERSISTENT_OBFUSCATED);
//    
//    // Save the purchase
//    Globals::save();
//    
//    // Animate indicator
//    _store_loading->add_anim_key()
//        .set_duration(0.5F)
//        .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
//
//}
//
//void KaiserMainMenu::store_failure_cb(std::string product_id)
//{
//    LOG_MESSAGE << "store_failure_cb " << product_id;
//    
//    // Animate indicator
//    _store_loading->add_anim_key()
//        .set_duration(0.5F)
//        .set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
//}
//
//void KaiserMainMenu::store_restore_cb(std::string product_id)
//{
//    LOG_MESSAGE << "store_restore_cb " << product_id;
//    store_success_cb(product_id);
//}


void KaiserMainMenu::begin_refresh_store (void)
{
    _store_scroller->remove_children();

    std::vector<std::string> products;
#if DT3_OS == DT3_ANDROID
    products.push_back("kaiserfullapp");
#else
    products.push_back("KAISERFULLAPP");
#endif
    
    // Animate indicator
    _store_loading->add_anim_key()
        .set_duration(0.5F)
        .set_color(Color4f(1.0F,1.0F,1.0F,1.0F));

    // Perform request
    RefCounter<std::shared_ptr<KaiserMainMenu>> *rc = new RefCounter<std::shared_ptr<KaiserMainMenu>>(checked_cast<KaiserMainMenu>(shared_from_this()));

    InAppPurchases::request_products_info(products, &KaiserMainMenu::end_refresh_store, rc);
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::show_error (std::string error)
{
    GUIAnimKey key;
    key.set_duration(0.5F);
    key.set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
    
    // Animate message
    _error_layout->add_anim_key(key);
}
    
void KaiserMainMenu::dismiss_error (void)
{
    GUIAnimKey key;
    key.set_duration(0.5F);
    key.set_color(Color4f(1.0F,1.0F,1.0F,0.0F));

    // Animate message
    _error_layout->add_anim_key(key);
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::strings_callback (std::map<std::string,std::string> &s, DTPerror error, void *data)
{
    if (data) {
        RefCounter<std::shared_ptr<KaiserMainMenu>> *rc = (RefCounter<std::shared_ptr<KaiserMainMenu>>*) data;
        std::shared_ptr<KaiserMainMenu> menu = rc->get();
        ASSERT(menu);
        
        menu->_ticker_text =    s["STRINGS_TICKER_TEXT"] + "             " +
                                s["STRINGS_TICKER_TEXT"] + "             " +
                                s["STRINGS_TICKER_TEXT"] + "             ";
        menu->_ticker_url =     s["STRINGS_TICKER_URL"];


        // Download Ads
        std::string ads_url = s["ADS_URL"];
        std::string ads_filename = s["ADS_FILENAME"];
        
        Globals::set_global("ADS_URL", ads_url, Globals::PERSISTENT);
        Globals::set_global("ADS_FILENAME", ads_filename, Globals::PERSISTENT);

        if (!ads_url.empty()) {
            KaiserDonkeyAds::download_ads ( URL(ads_url), FilePath("{SAVEDIR}/{ADS_FILENAME}") );
        }

        // Release ref counter
        rc->release();
    }
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::tick (const DTfloat dt)
{
    _angle += dt;
    
    _store_loading->set_orientation(Matrix3::set_rotation_z(_angle));
    _high_scores_loading->set_orientation(Matrix3::set_rotation_z(_angle));
    
    // Ticker
    _ticker_timer -= dt;
    if (_ticker_timer < 0.0F) {
        _ticker_timer += 0.2F;
        
        // Scroll
        if (_ticker_text.size() > 0) {
            DTcharacter c = _ticker_text.front();
            _ticker_text.erase(0,1);
            _ticker_text.push_back(c);
        }
        
        _ticker->set_label("{FMT_TICKER}"+_ticker_text);
    }
}

//==============================================================================
//==============================================================================

void KaiserMainMenu::add_to_world (World *world)
{		
	GUIController::add_to_world (world);
    
#ifndef DT3_EDITOR

    hide_ads();
    
    // Fetch strings
    auto rc = new RefCounter<std::shared_ptr<KaiserMainMenu>>(checked_cast<KaiserMainMenu>(shared_from_this()));
    DTPortal::strings(&KaiserMainMenu::strings_callback, rc);
    
    if (!System::renderer())
        return;
    
    DTfloat screen_width = System::renderer()->screen_width();
    DTfloat screen_height = System::renderer()->screen_height();

    DTboolean phone_mode = screen_height > screen_width;

    if (!phone_mode) {
        DTfloat scale = screen_width / 1024.0F;
        screen_width /= scale;
        screen_height /= scale;
    }

    // _______ _      _
    //|__   __(_)    | |            
    //   | |   _  ___| | _____ _ __ 
    //   | |  | |/ __| |/ / _ \ '__|
    //   | |  | | (__|   <  __/ |   
    //   |_|  |_|\___|_|\_\___|_|
    
    _ticker_layout = GUIGridLayout::create();
    _ticker_layout->set_rows_and_columns(2,1);
    _ticker_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _ticker_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 45.0F/screen_height));
    _ticker_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    
    // Ticker
    {
        _ticker = GUIObject::create();
        world->add_node_unique_name(_ticker);
        add_child(_ticker);
        
        _ticker_layout->add_item(1, 0, _ticker);
        
        _ticker->set_label("");
        _ticker->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _ticker->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_ticker));

        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _ticker->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_pressed_color(Color4f(1.0F,1.0F,1.0F,0.5F));
    }

    _ticker_layout->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));

    
    //             _
    //    /\      | |    
    //   /  \   __| |___ 
    //  / /\ \ / _` / __|
    // / ____ \ (_| \__ \
    ///_/    \_\__,_|___/
    //
    
    // Register the ads package
    KaiserDonkeyAds::register_ads_package (FilePath("{SAVEDIR}/{ADS_FILENAME}"));

    // If there are some ads, load them
    if (KaiserDonkeyAds::num_ads() > 0) {
        const KaiserDonkeyAds::Entry &e = KaiserDonkeyAds::get_ad(MoreMath::random_MT_int() % KaiserDonkeyAds::num_ads());
        
        _ads_layout = GUIGridLayout::create();
        _ads_layout->set_rows_and_columns(3,3);
        _ads_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_height));
        _ads_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, e.height/screen_height));
        _ads_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _ads_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
        _ads_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, e.width/screen_width));
        _ads_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

        // Ads
        {
            std::shared_ptr<GUIObject> widget = GUIObject::create();
            world->add_node_unique_name(widget);
            add_child(widget);
            
            _ads_layout->add_item(1, 1, widget);
            
            widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
            
            // Interaction 
            std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
            widget->add_component(widget_interaction);
            widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_ad, e.url));

            // Drawing
            std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
            widget->add_component(widget_drawing);
            
            widget_drawing->set_material(MaterialResource::import_resource(e.material));
            widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        }
        
        _ads_layout->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));
    }

#if DT3_OS != DT3_ANDROID

    //  _____            _       _   __  __          _ _
    // / ____|          (_)     | | |  \/  |        | (_)      
    //| (___   ___   ___ _  __ _| | | \  / | ___  __| |_  __ _ 
    // \___ \ / _ \ / __| |/ _` | | | |\/| |/ _ \/ _` | |/ _` |
    // ____) | (_) | (__| | (_| | | | |  | |  __/ (_| | | (_| |
    //|_____/ \___/ \___|_|\__,_|_| |_|  |_|\___|\__,_|_|\__,_|

    DTfloat sm_size = phone_mode ? 90.0F : 30.0F;

    _social_media_layout = GUIGridLayout::create();
    _social_media_layout->set_rows_and_columns(3,5);
    _social_media_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_height));
    _social_media_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, sm_size/screen_height));
    _social_media_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _social_media_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _social_media_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, sm_size/screen_width));
    _social_media_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_width));
    _social_media_layout->set_column_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, sm_size/screen_width));
    _social_media_layout->set_column_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_width));

    // Facebook
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
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
        add_child(widget);
        
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

#endif

    // __  __       _         __  __
    //|  \/  |     (_)       |  \/  |                 
    //| \  / | __ _ _ _ __   | \  / | ___ _ __  _   _ 
    //| |\/| |/ _` | | '_ \  | |\/| |/ _ \ '_ \| | | |
    //| |  | | (_| | | | | | | |  | |  __/ | | | |_| |
    //|_|  |_|\__,_|_|_| |_| |_|  |_|\___|_| |_|\__,_|

    DTfloat gap_size = phone_mode ? 120.0F : 75.0F;
    DTfloat mm_size = phone_mode ? 85.0F : 60.0F;

    DTfloat menu_bottom_size = 4 * mm_size;
    if (menu_bottom_size > 1.0F/3.0F*screen_height) {
        mm_size = mm_size * (1.0F/3.0F*screen_height) / menu_bottom_size;
    }

    _main_menu_layout = GUIGridLayout::create();
    _main_menu_layout->set_rows_and_columns(6,3);
    _main_menu_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, gap_size/screen_height));
    _main_menu_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, mm_size/screen_height));
    _main_menu_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, mm_size/screen_height));
    _main_menu_layout->set_row_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, mm_size/screen_height));
    _main_menu_layout->set_row_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, mm_size/screen_height));
    _main_menu_layout->set_row_policy(5, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _main_menu_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
    _main_menu_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _main_menu_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));

    // Play
    {
        _play = GUIObject::create();
        world->add_node_unique_name(_play);
        add_child(_play);
        
        _main_menu_layout->add_item(4, 1, _play);
        
        _play->set_label("{TITLE_PLAY}");
        _play->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _play->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_play));

        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _play->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_pressed_color(Color4f(1.0F,1.0F,1.0F,0.5F));
    }

    // High Scores
    {
        _high_scores = GUIObject::create();
        world->add_node_unique_name(_high_scores);
        add_child(_high_scores);
        
        _main_menu_layout->add_item(3, 1, _high_scores);
        
        _high_scores->set_label("{TITLE_HIGH_SCORES}");
        _high_scores->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _high_scores->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_high_scores));

        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _high_scores->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_pressed_color(Color4f(1.0F,1.0F,1.0F,0.5F));
    }

    // Store
    {
        _store = GUIObject::create();
        world->add_node_unique_name(_store);
        add_child(_store);
        
        _main_menu_layout->add_item(2, 1, _store);
        
        _store->set_label("{TITLE_STORE}");
        _store->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _store->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_store));

        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _store->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_pressed_color(Color4f(1.0F,1.0F,1.0F,0.5F));
    }

    // Instructions
    {
        _instructions = GUIObject::create();
        world->add_node_unique_name(_instructions);
        add_child(_instructions);
        
        _main_menu_layout->add_item(1, 1, _instructions);
        
        _instructions->set_label("{TITLE_INSTRUCTIONS}");
        _instructions->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        _instructions->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_instructions));

        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        _instructions->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_pressed_color(Color4f(1.0F,1.0F,1.0F,0.5F));
    }

    // ____             _
    //|  _ \           | |   
    //| |_) | __ _  ___| | __
    //|  _ < / _` |/ __| |/ /
    //| |_) | (_| | (__|   < 
    //|____/ \__,_|\___|_|\_\

    _back_layout = GUIGridLayout::create();
    _back_layout->set_rows_and_columns(2,2);
    _back_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    if (phone_mode) {
        _back_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 130.0F/screen_height));
    } else {
        _back_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 70.0F/screen_height));
    }

    _back_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_width));
    _back_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));

    // Back
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
        _back_layout->add_item(1, 0, widget);
        
        widget->set_label("{TITLE_BACK}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this, &type::click_back));

        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_pressed_color(Color4f(1.0F,1.0F,1.0F,0.5F));
    }

    // _    _ _       _
    //| |  | (_)     | |                                  
    //| |__| |_  __ _| |__    ___  ___ ___  _ __ ___  ___ 
    //|  __  | |/ _` | '_ \  / __|/ __/ _ \| '__/ _ \/ __|
    //| |  | | | (_| | | | | \__ \ (_| (_) | | |  __/\__ \
    //|_|  |_|_|\__, |_| |_| |___/\___\___/|_|  \___||___/
    //           __/ |                                    
    //          |___/
    
    _high_scores_layout = GUIGridLayout::create();
    _high_scores_layout->set_rows_and_columns(5,3);

    if (phone_mode) {
        _high_scores_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 130.0F/screen_height));
    } else {
        _high_scores_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));
    }
    _high_scores_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _high_scores_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_height));
    _high_scores_layout->set_row_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));
    _high_scores_layout->set_row_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));

    if (phone_mode) {
        _high_scores_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
        _high_scores_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _high_scores_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
    } else {
        _high_scores_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _high_scores_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 600.0F/screen_width));
        _high_scores_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    }

    // High Scores Title
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
        _high_scores_layout->add_item(3, 1, widget);
        
        widget->set_label("{TITLE_HIGH_SCORES_TITLE}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
        _high_scores_layout->add_item(1, 1, widget);

        widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg2.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Scroller
    {
        _high_scores_scroller = GUIObject::create();
        world->add_node_unique_name(_high_scores_scroller);
        add_child(_high_scores_scroller);
        
        _high_scores_layout->add_item(1, 1, _high_scores_scroller);
        
        _high_scores_scroller->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIScrollerLayout> widget_interaction = ComponentGUIScrollerLayout::create();
        _high_scores_scroller->add_component(widget_interaction);
        widget_interaction->set_scroll_horz(false);
        widget_interaction->set_material(MaterialResource::import_resource(FilePath("{ui_thumb.mat}")));
        widget_interaction->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }
    
    // Loading
    {
        _high_scores_loading = GUIObject::create();
        world->add_node_unique_name(_high_scores_loading);
        add_child(_high_scores_loading);
        
        _high_scores_layout->add_item(1, 1, _high_scores_loading, GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
        
        _high_scores_loading->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        _high_scores_loading->set_width(0.1F);
        _high_scores_loading->set_height(0.1F);
        _high_scores_loading->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        _high_scores_loading->set_size_mode(GUIObject::SIZE_MODE_HEIGHT_CONSTANT);

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _high_scores_loading->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_loading.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }


    //  _____ _
    // / ____| |                
    //| (___ | |_ ___  _ __ ___ 
    // \___ \| __/ _ \| '__/ _ \
    // ____) | || (_) | | |  __/
    //|_____/ \__\___/|_|  \___|
    
    
    std::shared_ptr<GUIGridLayout> store_restore_layout = GUIGridLayout::create();
    store_restore_layout->set_rows_and_columns(1,3);
    store_restore_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    store_restore_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    store_restore_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 400.0F/screen_width));
    store_restore_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    
    _store_layout = GUIGridLayout::create();
    _store_layout->set_rows_and_columns(7,3);

    if (phone_mode) {
        _store_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 130.0F/screen_height));
    } else {
        _store_layout->set_row_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));
    }
    _store_layout->set_row_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));
    _store_layout->set_row_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_height));
    _store_layout->set_row_policy(3, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _store_layout->set_row_policy(4, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 20.0F/screen_height));
    _store_layout->set_row_policy(5, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));
    _store_layout->set_row_policy(6, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));

    if (phone_mode) {
        _store_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));
        _store_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _store_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 10.0F/screen_width));

    } else {
        _store_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_width));
        _store_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _store_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 100.0F/screen_width));
    }

    _store_layout->add_item(1, 1, store_restore_layout);

    // Store Title
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
        _store_layout->add_item(5, 1, widget);
        
        widget->set_label("{TITLE_STORE}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
        _store_layout->add_item(3, 1, widget);

        widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_bg2.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        widget_drawing->set_draw_style(ComponentGUIDrawIcon::DRAW_STYLE_STRETCH_CENTER_2X2);
        widget_drawing->set_corner_height(0.025F);
        widget_drawing->set_corner_width(0.025F);
        
    }

    // Scroller
    {
        _store_scroller = GUIObject::create();
        world->add_node_unique_name(_store_scroller);
        add_child(_store_scroller);
        
        _store_layout->add_item(3, 1, _store_scroller);
        
        _store_scroller->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIScrollerLayout> widget_interaction = ComponentGUIScrollerLayout::create();
        _store_scroller->add_component(widget_interaction);
        widget_interaction->set_scroll_horz(false);
        widget_interaction->set_material(MaterialResource::import_resource(FilePath("{ui_thumb.mat}")));
        widget_interaction->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Loading
    {
        _store_loading = GUIObject::create();
        world->add_node_unique_name(_store_loading);
        add_child(_store_loading);
        
        _store_layout->add_item(3, 1, _store_loading, GUIGridLayoutItem::RESIZE_MODE_DO_NOTHING);
        
        _store_loading->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        _store_loading->set_width(0.1F);
        _store_loading->set_height(0.1F);
        _store_loading->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        _store_loading->set_size_mode(GUIObject::SIZE_MODE_HEIGHT_CONSTANT);

        // Drawing
        std::shared_ptr<ComponentGUIDrawIcon> widget_drawing = ComponentGUIDrawIcon::create();
        _store_loading->add_component(widget_drawing);
        
        widget_drawing->set_material(MaterialResource::import_resource(FilePath("{ui_loading.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
    }

    // Restore
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
        store_restore_layout->add_item(0,1,widget);
        
        widget->set_label("{TXT_STORE_RESTORE}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
        
        // Interaction 
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserMainMenu::click_restore));

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
    
    _error_layout = GUIGridLayout::create();
    _error_layout->set_rows_and_columns(5, 3);
    _error_layout->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _error_layout->set_row_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 75.0F/screen_height));
    _error_layout->set_row_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 64.0F/screen_height));
    _error_layout->set_row_policy(3, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 64.0F/screen_height));
    _error_layout->set_row_policy(4, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    if (phone_mode) {
        _error_layout->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 64.0F/screen_width));
        _error_layout->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _error_layout->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 64.0F/screen_width));
    } else {
        _error_layout->set_column_policy(0, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
        _error_layout->set_column_policy(1, GUILayoutPolicy(GUILayoutPolicy::POLICY_FIXED_SIZE, 400.0F/screen_width));
        _error_layout->set_column_policy(2, GUILayoutPolicy(GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    }

    std::shared_ptr<GUIGridLayout> error_layout_buttons = GUIGridLayout::create();
    error_layout_buttons->set_rows_and_columns(1, 3);
    error_layout_buttons->set_row_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    error_layout_buttons->set_column_policy(0, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    error_layout_buttons->set_column_policy(1, GUILayoutPolicy (GUILayoutPolicy::POLICY_FIXED_SIZE, 197.0F/screen_width));
    error_layout_buttons->set_column_policy(2, GUILayoutPolicy (GUILayoutPolicy::POLICY_WEIGHTED_SIZE, 1.0F));
    _error_layout->add_item(1, 1, error_layout_buttons);

    
    // Background
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
        _error_layout->set_border_item(widget, 0.05F, 0.05F);
        
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
        add_child(widget);
        
        _error_layout->add_item(3,1,widget);
        
        widget->set_label("{TXT_APP_STORE_ERROR}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Drawing
        std::shared_ptr<ComponentGUIDrawText> widget_drawing = ComponentGUIDrawText::create();
        widget->add_component(widget_drawing);
        
        widget_drawing->set_font_material(MaterialResource::import_resource(FilePath("{cards.mat}")));
        widget_drawing->set_shader(ShaderResource::import_resource(FilePath("{default.shdr}")));
        
    }
    
    
    // Dismiss
    {
        std::shared_ptr<GUIObject> widget = GUIObject::create();
        world->add_node_unique_name(widget);
        add_child(widget);
        
        error_layout_buttons->add_item(0,1,widget);
        
        widget->set_label("{DISMISS}");
        widget->set_color(Color4f(1.0F,1.0F,1.0F,0.0F));
        
        // Interaction
        std::shared_ptr<ComponentGUIButton> widget_interaction = ComponentGUIButton::create();
        widget->add_component(widget_interaction);
        widget_interaction->set_button_pressed_latent_call(make_latent_call(this,&KaiserMainMenu::dismiss_error));
        
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

    _error_layout->layout(Rectangle(0.0F,1.0F,0.0F,1.0F));

    set_state(STATE_MAIN_MENU, 0.0F);
#endif
    
    world->register_for_tick(this, make_callback(this, &type::tick));
}

void KaiserMainMenu::remove_from_world (void)
{
    world()->unregister_for_tick(this, make_callback(this, &type::tick));

    GUIController::remove_from_world();
}

//==============================================================================
//==============================================================================

} // DT3

