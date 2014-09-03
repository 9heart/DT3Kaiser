//==============================================================================
///	
///	File: KaiserWorld.cpp
///	
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "Kaiser/KaiserWorld.hpp"
#include "DT3Core/System/Factory.hpp"
#include "DT3Core/Types/FileBuffer/Archive.hpp"
#include "DT3Core/Types/FileBuffer/ArchiveData.hpp"
#include "DT3Core/World/WorldNode.hpp"
#include "DT3Core/Objects/PlaceableObject.hpp"
#include "DT3Core/Objects/GUIController.hpp"
#include "DT3Core/Components/ComponentDrawImagePlane.hpp"
#include "Kaiser/KaiserObjectBackground.hpp"
#include "Kaiser/KaiserMainMenu.hpp"
#include <algorithm>

//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
/// Register with node factory
//==============================================================================

IMPLEMENT_FACTORY_CREATION_WORLD(KaiserWorld)
		
//==============================================================================
/// Standard class constructors/destructors
//==============================================================================

KaiserWorld::KaiserWorld (void)	
{
	set_name("KaiserWorld");
}	

KaiserWorld::~KaiserWorld (void)
{    

}

//==============================================================================
//==============================================================================

void KaiserWorld::initialize (void)	
{
	World::initialize();
    
    _shadow = MaterialResource::import_resource(FilePath("{shadow.mat}"));
    _cards = MaterialResource::import_resource(FilePath("{atlas.mat}"));
    _shader = ShaderResource::import_resource(FilePath("{default.shdr}"));
}

//==============================================================================
//==============================================================================

void KaiserWorld::process_args (const std::string &args)
{
    if (args.find("launch_store") != std::string::npos) {

        // Check for main menu controller
        for (auto &i : nodes()) {

            std::shared_ptr<KaiserMainMenu> mm = checked_cast<KaiserMainMenu>(i);
            if (mm)
                mm->go_to_store();

        }

    }

}

//==============================================================================
//==============================================================================

void KaiserWorld::archive (const std::shared_ptr<Archive> &archive)
{
	World::archive(archive);

	archive->push_domain (class_id ());
    
	archive->pop_domain();
}

//==============================================================================
//==============================================================================

void KaiserWorld::draw (const DTfloat lag)
{
    _draw_batched_callbacks.sort(
        [](const DrawBatchedCallbackEntry &a, const DrawBatchedCallbackEntry &b){
            const GUIController *ga = checked_cast<const GUIController*>(a.node());
            const GUIController *gb = checked_cast<const GUIController*>(b.node());
            
            if (ga) return false;
            if (gb) return true;

            const PlaceableObject *pa = checked_cast<const PlaceableObject*>(a.node());
            const PlaceableObject *pb = checked_cast<const PlaceableObject*>(b.node());
        
            return pa->translation().z < pb->translation().z;
        }
    );

    _draw_callbacks.sort(
        [](const DrawCallbackEntry &a, const DrawCallbackEntry &b){
            const GUIController *ga = checked_cast<const GUIController*>(a.node());
            const GUIController *gb = checked_cast<const GUIController*>(b.node());
            
            if (ga) return false;
            if (gb) return true;

            const PlaceableObject *pa = checked_cast<const PlaceableObject*>(a.node());
            const PlaceableObject *pb = checked_cast<const PlaceableObject*>(b.node());
        
            return pa->translation().z < pb->translation().z;
        }
    );
    
    clean();
    
    if (!_camera)
        return;

    // Draw backgrounds
    for (auto draw_iterator = _draw_callbacks.begin(); draw_iterator != _draw_callbacks.end(); ++draw_iterator) {
        if (draw_iterator->node()->isa(KaiserObjectBackground::kind())) {
        
            // Change color on component
            ObjectBase* o = checked_cast<ObjectBase*>(draw_iterator->node());
            if (o) {
                std::shared_ptr<ComponentBase> c = o->component_by_type(ComponentBase::COMPONENT_DRAW);
                std::shared_ptr<ComponentDrawImagePlane> cd = checked_cast<ComponentDrawImagePlane>(c);
                cd->set_color(Color4f(1.0F,1.0F,1.0F,1.0F));
            }
        
            (*draw_iterator)(_camera, lag);
        }
    }

    // Draw shadows
    _b.batch_begin( _camera,
                    _shadow,
                    _shader,
                    Matrix4::identity(),
                    DT3GL_PRIM_TRIS,
                    DrawBatcher::FMT_V | DrawBatcher::FMT_T0 | DrawBatcher::FMT_C);

    for (auto draw_iterator = _draw_batched_shadow_callbacks.begin(); draw_iterator != _draw_batched_shadow_callbacks.end(); ++draw_iterator) {
        (*draw_iterator)(_camera, &_b, lag);
    }
    
    _b.batch_end();
    _b.flush();
    
    // Draw backgrounds again transparent
    for (auto draw_iterator = _draw_callbacks.begin(); draw_iterator != _draw_callbacks.end(); ++draw_iterator) {
        if (draw_iterator->node()->isa(KaiserObjectBackground::kind())) {
        
            // Change color on component
            ObjectBase* o = checked_cast<ObjectBase*>(draw_iterator->node());
            if (o) {
                std::shared_ptr<ComponentBase> c = o->component_by_type(ComponentBase::COMPONENT_DRAW);
                std::shared_ptr<ComponentDrawImagePlane> cd = checked_cast<ComponentDrawImagePlane>(c);
                cd->set_color(Color4f(1.0F,1.0F,1.0F,0.8F));
            }
        
            (*draw_iterator)(_camera, lag);
        }
    }

    // Draw cards
    _b.batch_begin( _camera,
                    _cards,
                    _shader,
                    Matrix4::identity(),
                    DT3GL_PRIM_TRIS,
                    DrawBatcher::FMT_V | DrawBatcher::FMT_T0 | DrawBatcher::FMT_C);

    for (auto draw_iterator = _draw_batched_callbacks.begin(); draw_iterator != _draw_batched_callbacks.end(); ++draw_iterator) {
        (*draw_iterator)(_camera, &_b, lag);
    }

    _b.flush();
    
    // Draw everything else
    for (auto draw_iterator = _draw_callbacks.begin(); draw_iterator != _draw_callbacks.end(); ++draw_iterator) {
        if (!draw_iterator->node()->isa(KaiserObjectBackground::kind())) {
            (*draw_iterator)(_camera, lag);
        }
    }


}

//==============================================================================
//==============================================================================

void KaiserWorld::clean (void)
{
    //
    // Clean up callbacks
    //
    
    for(auto &d : _draw_batched_shadow_callbacks_to_delete) {
        auto dd = std::remove(_draw_batched_shadow_callbacks.begin(), _draw_batched_shadow_callbacks.end(), d);
        _draw_batched_shadow_callbacks.erase(dd,_draw_batched_shadow_callbacks.end());
    }

    for(auto &d : _draw_batched_callbacks_to_delete) {
        auto dd = std::remove(_draw_batched_callbacks.begin(), _draw_batched_callbacks.end(), d);
        _draw_batched_callbacks.erase(dd,_draw_batched_callbacks.end());
    }

    _draw_batched_shadow_callbacks_to_delete.clear();
    _draw_batched_callbacks_to_delete.clear();
}

//==============================================================================
//==============================================================================

void KaiserWorld::register_for_shadow_draw_batched (WorldNode *node, std::shared_ptr<DrawBatchedCallbackType> cb)
{
    ASSERT(std::find(_draw_batched_shadow_callbacks.begin(), _draw_batched_shadow_callbacks.end(), DrawBatchedCallbackEntry(node, cb)) == _draw_batched_shadow_callbacks.end());
    _draw_batched_shadow_callbacks.emplace_back(node, cb);
}

void KaiserWorld::unregister_for_shadow_draw_batched (WorldNode *node, std::shared_ptr<DrawBatchedCallbackType> cb)
{
    _draw_batched_shadow_callbacks_to_delete.emplace_back(node, cb);
}

//==============================================================================
//==============================================================================

void KaiserWorld::register_for_draw_batched (WorldNode *node, std::shared_ptr<DrawBatchedCallbackType> cb)
{
    ASSERT(std::find(_draw_batched_callbacks.begin(), _draw_batched_callbacks.end(), DrawBatchedCallbackEntry(node, cb)) == _draw_batched_callbacks.end());
    _draw_batched_callbacks.emplace_back(node, cb);
}

void KaiserWorld::unregister_for_draw_batched (WorldNode *node, std::shared_ptr<DrawBatchedCallbackType> cb)
{
    _draw_batched_callbacks_to_delete.emplace_back(node, cb);
}

//==============================================================================
//==============================================================================

} // DT3

