#ifndef DT3_KAISERINTERACTION
#define DT3_KAISERINTERACTION
//==============================================================================
///	
///	File: KaiserInteraction.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/Components/ComponentBase.hpp"
#include "DT3Core/Types/Node/Plug.hpp"
#include "DT3Core/Types/Node/Event.hpp"
#include "DT3Core/Types/Math/Vector3.hpp"
#include "DT3Core/Types/Utility/Callback.hpp"

namespace DT3 {

//==============================================================================
/// Forward declarations
//==============================================================================

class GUITouchEvent;
class Vector3;
class KaiserObjectCard;

//==============================================================================
/// GUI behaviour for a button.
//==============================================================================

class KaiserInteraction: public ComponentBase {
    public:
        DEFINE_TYPE(KaiserInteraction,ComponentBase)
		DEFINE_CREATE_AND_CLONE
		DEFINE_PLUG_NODE

                                    KaiserInteraction       (void);
									KaiserInteraction       (const KaiserInteraction &rhs);
        KaiserInteraction &         operator =              (const KaiserInteraction &rhs);
        virtual                     ~KaiserInteraction      (void);
    
        virtual void                archive                 (const std::shared_ptr<Archive> &archive);
		
	public:
		/// Called to initialize the object
		virtual void				initialize              (void);
        
		/// Returns the component type. This defines which slot the component is
        /// put into on the object.
		/// \return Component type
        virtual ComponentType       component_type          (void)  {   return COMPONENT_TOUCH;  }

		/// Called when this component is added to the owner. Note that this will
        /// only be called if the owner is added to a world already. If not it 
        /// will be called when it is added to the World.
		/// \param owner Pointer to the owner
		virtual void                add_to_owner            (ObjectBase *owner);

		/// Called when this component is removed from its owner.
		virtual void                remove_from_owner       (void);


		/// Callback called when the component is getting a touch begin event
		/// \param event Touch events
        void                        touches_began           (GUITouchEvent *event);

		/// Callback called when the component is getting a touch move event
		/// \param event Touch events
        void                        touches_moved           (GUITouchEvent *event);

		/// Callback called when the component is getting a touch end event
		/// \param event Touch events
        void                        touches_ended           (GUITouchEvent *event);

		/// Callback called when the component is getting a touch cancelled event
		/// \param event Touch events
        void                        touches_cancelled       (GUITouchEvent *event);
    
		/// Allow dragging of cards
        DEFINE_ACCESSORS(allow_interactions, set_allow_interactions, DTboolean, _allow_interactions)

        typedef std::shared_ptr<Callback<std::shared_ptr<KaiserObjectCard>, DTboolean>> tcb;
        typedef std::shared_ptr<Callback<std::shared_ptr<KaiserObjectCard>>> cb;

		/// Allow dragging of cards
        DEFINE_ACCESSORS(played_cb, set_played_cb, cb, _cb)

		/// Allow clicking tricks
        DEFINE_ACCESSORS(trick_cb, set_trick_cb, tcb, _trick_cb)

    private:
        void                        event_to_ray            (GUITouchEvent *event, Vector3 &src, Vector3 &dst);
        DTboolean                   card_intersection_pos   (std::shared_ptr<KaiserObjectCard> card, const Vector3 &src, const Vector3 &dst, Vector3 &pos);

        DTboolean                                                       _allow_interactions;
    
        tcb _trick_cb;
        cb  _cb;

        Vector3                                                         _card_pos;
        std::shared_ptr<KaiserObjectCard>                               _card;

};

//==============================================================================
//==============================================================================

} // DT3

#endif
