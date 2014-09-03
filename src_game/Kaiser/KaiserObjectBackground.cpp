//==============================================================================
///	
///	File: KaiserObjectBackground.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserObjectBackground.hpp"
#include "DT3Core/System/Factory.hpp"
#include "DT3Core/Types/FileBuffer/Archive.hpp"
#include "DT3Core/Types/FileBuffer/ArchiveData.hpp"

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
/// Register with object factory
//==============================================================================

IMPLEMENT_FACTORY_CREATION_PLACEABLE(KaiserObjectBackground,"Kaiser","EdCameraObjectAdapter")
IMPLEMENT_PLUG_NODE(KaiserObjectBackground)

//==============================================================================
//==============================================================================

BEGIN_IMPLEMENT_PLUGS(KaiserObjectBackground)
END_IMPLEMENT_PLUGS

//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserObjectBackground::KaiserObjectBackground (void)
{

}
		
KaiserObjectBackground::KaiserObjectBackground (const KaiserObjectBackground &rhs)
    :   PlaceableObject         (rhs)
{

}

KaiserObjectBackground & KaiserObjectBackground::operator = (const KaiserObjectBackground &rhs)
{
    // Make sure we are not assigning the class to itself
    if (&rhs != this) { 
		PlaceableObject::operator = (rhs);
    }
    return (*this);
}
			
KaiserObjectBackground::~KaiserObjectBackground (void)
{	

}

//==============================================================================
//==============================================================================

void KaiserObjectBackground::archive (const std::shared_ptr<Archive> &archive)
{
    PlaceableObject::archive(archive);

	archive->push_domain (class_id ());
    archive->pop_domain ();
}

//==============================================================================
//==============================================================================

} // DT3

