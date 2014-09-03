#ifndef DT3_KAISERWORLD
#define DT3_KAISERWORLD
//==============================================================================
///	
///	File: KaiserWorld.hpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "DT3Core/World/World.hpp"
#include "DT3Core/Types/Graphics/DrawBatcher.hpp"

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
/// Forward declarations
//==============================================================================

typedef Callback<const std::shared_ptr<CameraObject> &, DrawBatcher *, DTfloat> DrawBatchedCallbackType;
typedef WorldEntry<DrawBatchedCallbackType> DrawBatchedCallbackEntry;

//==============================================================================
//==============================================================================

class KaiserWorld: public World {
    public:
        DEFINE_TYPE(KaiserWorld,World)
		DEFINE_CREATE

									KaiserWorld             (void);
	private:
									KaiserWorld             (const KaiserWorld &rhs);
        KaiserWorld &               operator =              (const KaiserWorld &rhs);
	public:
        virtual						~KaiserWorld            (void);
                                        
        virtual void				archive                 (const std::shared_ptr<Archive> &archive);

    public:
		/// Called to initialize the object
		virtual void				initialize              (void);

		/// Process arguments
		virtual void                process_args            (const std::string &args);

		/// Description
		/// \param param description
		/// \return description
		virtual void                draw                    (const DTfloat lag);

		/// Description
		/// \param param description
		/// \return description
        virtual void                clean                   (void);

        /// Description
		/// \param param description
		/// \return description
        virtual void                register_for_draw_batched    (WorldNode *node, std::shared_ptr<DrawBatchedCallbackType> cb);
        virtual void                unregister_for_draw_batched  (WorldNode *node, std::shared_ptr<DrawBatchedCallbackType> cb);

        /// Description
		/// \param param description
		/// \return description
        virtual void                register_for_shadow_draw_batched    (WorldNode *node, std::shared_ptr<DrawBatchedCallbackType> cb);
        virtual void                unregister_for_shadow_draw_batched  (WorldNode *node, std::shared_ptr<DrawBatchedCallbackType> cb);

    private:
        DrawBatcher                         _b;
    
        std::shared_ptr<MaterialResource>   _shadow;
        std::shared_ptr<MaterialResource>   _cards;
        std::shared_ptr<ShaderResource>     _shader;

        std::list<DrawBatchedCallbackEntry> _draw_batched_callbacks;
        std::list<DrawBatchedCallbackEntry> _draw_batched_callbacks_to_delete;

        std::list<DrawBatchedCallbackEntry> _draw_batched_shadow_callbacks;
        std::list<DrawBatchedCallbackEntry> _draw_batched_shadow_callbacks_to_delete;
};

//==============================================================================
//==============================================================================

} // DT3

#endif
