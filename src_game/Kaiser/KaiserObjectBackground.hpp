#ifndef DT3_KAISEROBJECTBACKGROUND
#define DT3_KAISEROBJECTBACKGROUND
//==============================================================================
///	
///	File: KaiserObjectBackground.hpp
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

//==============================================================================
/// Base object for the different placeable types of objects in the engine.
//==============================================================================

class KaiserObjectBackground: public PlaceableObject {
    public:
        DEFINE_TYPE(KaiserObjectBackground,PlaceableObject)
		DEFINE_CREATE_AND_CLONE
		DEFINE_PLUG_NODE
		
                                    KaiserObjectBackground  (void);
                                    KaiserObjectBackground  (const KaiserObjectBackground &rhs);
        KaiserObjectBackground &    operator =				(const KaiserObjectBackground &rhs);
        virtual                     ~KaiserObjectBackground (void);
        
        virtual void                archive                 (const std::shared_ptr<Archive> &archive);
  
	public:
};

//==============================================================================
//==============================================================================

} // DT3

#endif
