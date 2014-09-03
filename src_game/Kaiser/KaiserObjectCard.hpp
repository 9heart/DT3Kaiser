#ifndef DT3_KAISEROBJECTCARD
#define DT3_KAISEROBJECTCARD
//==============================================================================
///	
///	File: KaiserObjectCard.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/Objects/PlaceableObject.hpp"
#include "DT3Core/Types/Graphics/DrawBatcher.hpp"
#include "Kaiser/KaiserCommon.hpp"

namespace DT3 {

//==============================================================================
/// Forward declarations
//==============================================================================

class CameraObject;
class MaterialResource;
class ShaderResource;
class FilePath;
class DrawBatcher;

//==============================================================================
/// Base object for the different placeable types of objects in the engine.
//==============================================================================

class KaiserObjectCard: public PlaceableObject {
    public:
        DEFINE_TYPE(KaiserObjectCard,PlaceableObject)
		DEFINE_CREATE_AND_CLONE
		DEFINE_PLUG_NODE
		
                                    KaiserObjectCard		(void);
                                    KaiserObjectCard		(const KaiserObjectCard &rhs);
        KaiserObjectCard &          operator =				(const KaiserObjectCard &rhs);
        virtual                     ~KaiserObjectCard		(void);
        
        virtual void                archive                 (const std::shared_ptr<Archive> &archive);
  
	public:
		/// Called to initialize the object
		virtual void				initialize				(void);
    
        /// Initialize the card
        void                        load                    (const FilePath &material_path, const std::string &name, Suit suit, Card value);
		
		/// Draw Callback for component
		/// \param camera Camera used for drawing
        /// \param lag frame lag
        void                        draw                    (const std::shared_ptr<CameraObject> &camera, DrawBatcher *b, const DTfloat lag);

		/// Draw Callback for component
		/// \param camera Camera used for drawing
        /// \param lag frame lag
        void                        draw_shadow             (const std::shared_ptr<CameraObject> &camera, DrawBatcher *b, const DTfloat lag);

		/// Object was added to a world
		/// world world that object was added to
        virtual void                add_to_world            (World *world);

		/// Object was removed from a world
        virtual void                remove_from_world       (void);

		/// Save the current transform
        void                        save_transform          (void)  {   _save_transform = transform();  }

		/// Restore current transform
        void                        restore_transform       (void);
    
    


        // Accessors for suit
        DEFINE_ACCESSORS(suit, set_suit, Suit, _suit)

        // Accessors for value
        DEFINE_ACCESSORS(value, set_value, Card, _value)

        // Accessors for interactions
        DEFINE_ACCESSORS(can_interact, set_can_interact, DTboolean, _can_interact)

        // Accessors for interactions
        DEFINE_ACCESSORS(can_use, set_can_use, DTboolean, _can_use)

        // Accessors for interactions
        DEFINE_ACCESSORS(is_trick, set_trick, DTboolean, _is_trick)

    private:    
        DTboolean                           _can_interact;
        DTboolean                           _can_use;

        DTboolean                           _is_trick;

        Suit                                _suit;
        Card                                _value;
        
        Matrix4                             _save_transform;
        Rectangle                           _texcoords;
        Rectangle                           _texcoords_back;

};

//==============================================================================
//==============================================================================

inline DTboolean is_greater_than (  const std::shared_ptr<KaiserObjectCard> &a,
                                    const std::shared_ptr<KaiserObjectCard> &b,
                                    Suit trump, Suit first_played, Suit hint)
{
    DTint as,bs;

    if (a->suit() == trump)             as = 3;
    else if (a->suit() == first_played) as = 2;
    else if (a->suit() == hint)         as = 1;
    else                                as = 0;

    if (b->suit() == trump)             bs = 3;
    else if (b->suit() == first_played) bs = 2;
    else if (b->suit() == hint)         bs = 1;
    else                                bs = 0;

    DTint av = (as << 16) | a->value();
    DTint bv = (bs << 16) | b->value();

    return av > bv;
}

inline DTboolean is_equal ( const std::shared_ptr<KaiserObjectCard> &a,
                            const std::shared_ptr<KaiserObjectCard> &b,
                            Suit trump, Suit first_played, Suit hint)
{
    DTint as,bs;

    if (a->suit() == trump)             as = 3;
    else if (a->suit() == first_played) as = 2;
    else if (a->suit() == hint)         as = 1;
    else                                as = 0;

    if (b->suit() == trump)             bs = 3;
    else if (b->suit() == first_played) bs = 2;
    else if (b->suit() == hint)         bs = 1;
    else                                bs = 0;

    DTint av = (as << 16) | a->value();
    DTint bv = (bs << 16) | b->value();

    return av == bv;
}

//==============================================================================
//==============================================================================

inline DTint rank_card (const std::shared_ptr<KaiserObjectCard> &a, Suit trump, Suit first_played, Suit hint)
{
    if (!a) return 0;

    DTint as;

    if (a->suit() == trump)             as = 3;
    else if (a->suit() == first_played) as = 2;
    else if (a->suit() == hint)         as = 1;
    else                                as = 0;

    DTint av = (as << 16) | a->value();

    return av;
}

//==============================================================================
//==============================================================================

} // DT3

#endif
