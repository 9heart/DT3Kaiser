//==============================================================================
///	
///	File: AppNative.cpp
///
/// Copyright (C) 2000-2013 by Smells Like Donkey, Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "AppNative.hpp"
#include "DT3Core/Types/Base/BaseInclude.hpp"
#include "DT3Core/Types/Math/Vector3.hpp"
#include "DT3Core/Entry/GameMainThread.hpp"
#include "DT3Core/System/FileManager.hpp"
#include "DT3Core/Types/FileBuffer/FileHandle.hpp"
#include "DT3Core/Types/FileBuffer/FileHandleUncompressedFD.hpp"
#include "DT3Core/Types/FileBuffer/FileHandleCompressedFD.hpp"
#include "DT3Core/Types/FileBuffer/FilePath.hpp"
#include "DT3Core/System/Globals.hpp"
#include "DT3Core/System/SystemCallbacks.hpp"
#include "DT3Core/Types/Utility/ConsoleStream.hpp"
#include "DT3Core/Types/Utility/Analytics.hpp"
#include DT3_HAL_INCLUDE_PATH

#include <jni.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#include <android/sensor.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <android_native_app_glue.h>

using namespace DT3;

//==============================================================================
// Shared state for our app.
//==============================================================================

struct Engine {
    struct android_app*	app;

    ASensorManager* 	sensorManager;
    
    const ASensor* 		accelerometerSensor;
    const ASensor* 		magnetometerSensor;
    const ASensor* 		gyroSensor;
    
    ASensorEventQueue* 	sensorEventQueue;
    
    DT3::TouchEvent  	touchEvent;

    int 				animating;
    EGLDisplay 			display;
    EGLSurface 			surface;
    EGLContext 			context;
    int32_t 			width;
    int32_t 			height;
};
Engine gEngine;

//==============================================================================
//==============================================================================

void show_twitter (const std::string &msg)
{

}

void show_facebook (const std::string &msg)
{

}

void show_ads (void)
{
   // Get the android application's activity.
    ANativeActivity* activity = gEngine.app->activity;
    JavaVM* jvm = activity->vm;

    JNIEnv* env = NULL;
    DTboolean needs_detach = false;
    int env_stat = jvm->GetEnv( (void**) &env, JNI_VERSION_1_6);
    if (env_stat == JNI_EDETACHED) {
        jvm->AttachCurrentThread(&env, 0);
        needs_detach = true;
    }
    ASSERT(env);

    jclass c_activity = env->GetObjectClass(activity->clazz);
    jmethodID m_show_ad_popup = env->GetMethodID(c_activity, "showAdPopup", "()V");

    env->CallVoidMethod(activity->clazz, m_show_ad_popup);

    if (needs_detach)
        jvm->DetachCurrentThread();
}

void hide_ads (void)
{
    // Get the android application's activity.
    ANativeActivity* activity = gEngine.app->activity;
    JavaVM* jvm = activity->vm;

    JNIEnv* env = NULL;
    DTboolean needs_detach = false;
    int env_stat = jvm->GetEnv( (void**) &env, JNI_VERSION_1_6);
    if (env_stat == JNI_EDETACHED) {
        jvm->AttachCurrentThread(&env, 0);
        needs_detach = true;
    }
    ASSERT(env);

    jclass c_activity = env->GetObjectClass(activity->clazz);
    ASSERT(c_activity);
    jmethodID m_hide_ad_popup = env->GetMethodID(c_activity, "hideAdPopup", "()V");
    ASSERT(m_hide_ad_popup);

    env->CallVoidMethod(activity->clazz, m_hide_ad_popup);
    
    if (needs_detach)
        jvm->DetachCurrentThread();
}

//==============================================================================
// Define hooks from the engine
//==============================================================================

void screen_inited (void)
{
    // In order to render with OpenGL from another thread, this has
    // to be called in the thread. Conveniently, this callback will
    // be called when the screen has been opened.
    //[gGLView makeCurrentContext];
}

void screen_closed (void)
{
    // Screen is invalid so clear the current context
    //[gGLView clearCurrentContext];
}

void screen_changed (DTuint width, DTuint height)
{
    // In order to render with OpenGL from another thread, this has
    // to be called in the thread. Conveniently, this callback will
    // be called when the screen has been opened.
    //[gGLView makeCurrentContext];
}

void screen_swap (void)
{
    //[gGLView swapBuffers];
    ::eglSwapBuffers(gEngine.display, gEngine.surface);
}

//==============================================================================
// Iterate all of the assets in the root
//==============================================================================

static void fileOpenCB (const FilePath &path, std::shared_ptr<FileHandle> &file_handle, DTsize &start, DTsize &length, void *data)
{
    // Check if it's an asset
    if (path.full_path().substr(0,6) != "Asset@") {
        file_handle = NULL;
        return;
    }

    // State
    struct android_app* state = (struct android_app*) data;
    
    // Get the asset manager
    AAssetManager* mgr = state->activity->assetManager;


    // Open Files    
    if (path.in_package()) {
        // In package
        std::string full_path = path.package_path();
        full_path = full_path.substr(6);    // Strip off "Asset@"
        
        LOG_MESSAGE << "Opening Asset from Package: " << full_path;

        // Get the asset from the asset manager
        AAsset* asset = AAssetManager_open(mgr, full_path.c_str(), AASSET_MODE_UNKNOWN);
    
        // Get a file descriptor for the asset
        off_t outStart, outLength;
        int fd = AAsset_openFileDescriptor(asset, &outStart, &outLength);

        // Close the asset
        AAsset_close(asset);
        
        // Error check
        if (fd < 0) {
            file_handle = NULL;
            return;
        }

        // Find file in package
        FilePath packagename;
        DTsize file_start, file_length, file_uncompressed_length;
        FileManager::start_and_length(path, packagename, file_start, file_length, file_uncompressed_length);

        LOG_MESSAGE << " Asset: " << path.full_path();
        LOG_MESSAGE << " Start: " << file_start;
        LOG_MESSAGE << " Length: " << file_length;
        LOG_MESSAGE << " Uncompressed Length: " << file_uncompressed_length;

        if (file_length == file_uncompressed_length) {
            auto fh = FileHandleUncompressedFD::create();
            fh->set_fd(fd);

            file_handle = fh;
            start = file_start;
            length = file_length;

        } else {
            auto fh = FileHandleCompressedFD::create();
            fh->set_fd(fd,file_start,file_length,file_uncompressed_length);

            file_handle = fh;
            start = 0;
            length = file_uncompressed_length;
        }

    } else {
        // Not in Package
        std::string full_path = path.full_path();
        full_path = full_path.substr(6);    // Strip off "Asset@"
        
        LOG_MESSAGE << "Opening Asset: " << full_path;

        // Get the asset from the asset manager
        AAsset* asset = AAssetManager_open(mgr, full_path.c_str(), AASSET_MODE_UNKNOWN);
    
        // Get a file descriptor for the asset
        off_t outStart, outLength;
        int fd = AAsset_openFileDescriptor(asset, &outStart, &outLength);

        // Close the asset
        AAsset_close(asset);

        // Error check
        if (fd < 0) {
            file_handle = NULL;
            return;
        }
        
        auto fh = FileHandleUncompressedFD::create();
        fh->set_fd(fd);

        file_handle = fh;
        start = outStart;
        length = outLength;
    }
        
}

static void assetScan (AAssetManager* mgr)
{
    LOG_MESSAGE << "assetScan called";

    AAssetDir* assetDir = AAssetManager_openDir(mgr, "");
    
    const char* filename;
    
    while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
        LOG_MESSAGE << "Asset: " << filename;
        DT3::Globals::set_global(filename, std::string("Asset@") + filename, DT3::Globals::READ_ONLY);
    }
    AAssetDir_close(assetDir);
}

//==============================================================================
// Initialize an EGL context for the current display.
//==============================================================================

static int engineInitDisplay(struct Engine* engine) {
    LOG_MESSAGE << "engineInitDisplay called";
    
    EGLDisplay display = ::eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ::eglInitialize(display, 0, 0);

    // Get Configs
    GLint num_configs;
    GLenum result = ::eglGetConfigs(display,NULL,0,&num_configs);
    ASSERT(result != EGL_FALSE );
    
    LOG_MESSAGE << "number of configs found " << num_configs;
    
    // Now we create a buffer to store all our configs
    EGLConfig *configs = new EGLConfig[num_configs];
    result = ::eglGetConfigs(display,configs,num_configs,&num_configs);
    ASSERT(result != EGL_FALSE );

    for(DTint i=0; i < num_configs; ++i) {
        GLint value;
    
        LOG_MESSAGE << "Config #" << i;
        ::eglGetConfigAttrib(display,configs[i],EGL_BUFFER_SIZE,&value);
        LOG_MESSAGE << "Buffer Size " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_RED_SIZE,&value);
        LOG_MESSAGE << "Red Size " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_GREEN_SIZE,&value);
        LOG_MESSAGE << "Green Size " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_BLUE_SIZE,&value);
        LOG_MESSAGE << "Blue Size " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_ALPHA_SIZE,&value);
        LOG_MESSAGE << "Alpha Size " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_CONFIG_CAVEAT,&value);

        switch(value) {
            case EGL_NONE:        LOG_MESSAGE << "EGL_CONFIG_CAVEAT EGL_NONE\n";          break;
            case EGL_SLOW_CONFIG: LOG_MESSAGE << "EGL_CONFIG_CAVEAT EGL_SLOW_CONFIG\n";   break;
        }
        
        ::eglGetConfigAttrib(display,configs[i],EGL_CONFIG_ID,&value);
        LOG_MESSAGE << "Config ID " << value;

        ::eglGetConfigAttrib(display,configs[i],EGL_DEPTH_SIZE,&value);
        LOG_MESSAGE << "Depth size " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_STENCIL_SIZE,&value);
        LOG_MESSAGE << "Stencil size " << value;

        ::eglGetConfigAttrib(display,configs[i],EGL_MAX_PBUFFER_WIDTH,&value);
        LOG_MESSAGE << "Max pbuffer width " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_MAX_PBUFFER_HEIGHT,&value);
        LOG_MESSAGE << "Max pbuffer height " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_MAX_PBUFFER_PIXELS,&value);
        LOG_MESSAGE << "Max pbuffer pixels " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_NATIVE_RENDERABLE,&value);
        LOG_MESSAGE << "Native renderable " << std::string(value ? "true" : "false");
        ::eglGetConfigAttrib(display,configs[i],EGL_NATIVE_VISUAL_ID,&value);
        LOG_MESSAGE << "Native visual ID " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_NATIVE_VISUAL_TYPE,&value);
        LOG_MESSAGE << "Native visual type " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_SAMPLE_BUFFERS,&value);
        LOG_MESSAGE << "Sample Buffers " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_SAMPLES,&value);
        LOG_MESSAGE << "Samples " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_SURFACE_TYPE,&value);
        LOG_MESSAGE << "Surface type " << value;
        ::eglGetConfigAttrib(display,configs[i],EGL_TRANSPARENT_TYPE,&value);
    }

    delete[] configs;

    // Initialize OpenGL ES and EGL

    // Here specify the attributes of the desired configuration.
    // Below, we select an EGLConfig with at least 8 bits per color
    // component compatible with on-screen windows
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, 	    EGL_WINDOW_BIT,
            EGL_BLUE_SIZE,		    8,
            EGL_GREEN_SIZE,		    8,
            EGL_RED_SIZE,		    8,
            EGL_ALPHA_SIZE,		    8,
            EGL_DEPTH_SIZE,		    24,
            EGL_STENCIL_SIZE,	    8,
            EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };
    
    EGLint w, h, format;
    
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    // Here, the application chooses the configuration it desires. In this
    // sample, we have a very simplified selection process, where we pick
    // the first EGLConfig that matches our criteria
    ::eglChooseConfig(display, attribs, &config, 1, &num_configs);

    // EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
    // guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
    // As soon as we picked a EGLConfig, we can safely reconfigure the
    // ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
    ::eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = ::eglCreateWindowSurface(display, config, engine->app->window, NULL);
    
    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    context = ::eglCreateContext(display, config, NULL, context_attribs);

    if (::eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        //LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    ::eglQuerySurface(display, surface, EGL_WIDTH, &w);
    ::eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    // Initialize GL state.
    engine->animating = 1;
    
    // Small Screen
    if (w < h) {
        Globals::set_global("SYS_SMALL_SCREEN", "1", Globals::VOLATILE);
    } else {
        Globals::set_global("SYS_SMALL_SCREEN", "0", Globals::VOLATILE);
    }

    return 0;
}

//==============================================================================
// Tear down the EGL context currently associated with the display.
//==============================================================================

static void engineTermDisplay(struct Engine* engine) {
    LOG_MESSAGE << "engineTermDisplay called";

    if (engine->display != EGL_NO_DISPLAY) {
        ::eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (engine->context != EGL_NO_CONTEXT) {
            ::eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            ::eglDestroySurface(engine->display, engine->surface);
        }
        ::eglTerminate(engine->display);
    }
    
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

//==============================================================================
// Process the next input event.
//==============================================================================

static int32_t engineHandleInput(struct android_app* app, AInputEvent* event) {
    LOG_MESSAGE << "engineHandleInput called";

    struct Engine* engine = (struct Engine*)app->userData;
    
    int32_t type = AInputEvent_getType(event);
    
    // Motion event
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        
        int32_t index = (AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		int32_t action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

        if (index < DT3::TouchEvent::MAX_NUM_TOUCHES) {
        
        	Vector2 location( AMotionEvent_getX(event, index), AMotionEvent_getY(event, index) );
        	
        	LOG_MESSAGE << "Location: " << index << "," << location.x << "," << location.y;
        
			if ( action == AMOTION_EVENT_ACTION_DOWN) {
			    LOG_MESSAGE << "AMOTION_EVENT_ACTION_DOWN";
		
				engine->touchEvent.touches[index].state = DT3::TouchEvent::STATE_PRESSED;
				engine->touchEvent.touches[index].pos = location;
				engine->touchEvent.touches[index].previous_pos = location;
				engine->touchEvent.touches[index].first_pos = location;
				engine->touchEvent.touches[index].delta = Vector2(0.0F,0.0F);
				engine->touchEvent.touches[index].dt = 0.0F;
				engine->touchEvent.touches[index].velocity = Vector2(0.0F,0.0F);

				engine->touchEvent.touches[index].timer.delta_time();
			
				GameMainThread::touch_event(engine->touchEvent);

			} else if (action == AMOTION_EVENT_ACTION_UP) {
			    LOG_MESSAGE << "AMOTION_EVENT_ACTION_UP";

				engine->touchEvent.touches[index].state = DT3::TouchEvent::STATE_RELEASED;
				engine->touchEvent.touches[index].previous_pos = engine->touchEvent.touches[index].pos;
				engine->touchEvent.touches[index].pos = location;
				engine->touchEvent.touches[index].delta = engine->touchEvent.touches[index].pos - engine->touchEvent.touches[index].previous_pos;
				engine->touchEvent.touches[index].dt = engine->touchEvent.touches[index].timer.delta_time();
				engine->touchEvent.touches[index].velocity = engine->touchEvent.touches[index].delta / engine->touchEvent.touches[index].dt;

				GameMainThread::touch_event(engine->touchEvent);

				engine->touchEvent.touches[index].state = TouchEvent::STATE_NONE;

			} else if (action == AMOTION_EVENT_ACTION_MOVE) {
                LOG_MESSAGE << "AMOTION_EVENT_ACTION_MOVE";

                engine->touchEvent.touches[index].state = DT3::TouchEvent::STATE_DOWN;
                engine->touchEvent.touches[index].previous_pos = engine->touchEvent.touches[index].pos;
                engine->touchEvent.touches[index].pos = location;
                engine->touchEvent.touches[index].delta = engine->touchEvent.touches[index].pos - engine->touchEvent.touches[index].previous_pos;
                engine->touchEvent.touches[index].dt = engine->touchEvent.touches[index].timer.delta_time();
                engine->touchEvent.touches[index].velocity = engine->touchEvent.touches[index].delta / engine->touchEvent.touches[index].dt;

                GameMainThread::touch_event(engine->touchEvent);
			}
			
		}
        
        return 1;
        
    // Two soft buttons
    } else if (type == AINPUT_EVENT_TYPE_KEY) {
    	int32_t keycode = AKeyEvent_getKeyCode(event);
    
  		if (keycode == AKEYCODE_BACK) {
			return 0; //DT3::GameMainThread::do_back_button();
    	} else if (keycode == AKEYCODE_MENU) {
			DT3::GameMainThread::do_menu_button();
    	}
    	
        return 1;
    }
    
    return 0;
}

//==============================================================================
// Process the next main command.
//==============================================================================

static void engineHandleCommand(struct android_app* app, int32_t cmd) {

    struct Engine* engine = (struct Engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            LOG_MESSAGE << "engineHandleCommand called APP_CMD_SAVE_STATE";

            // The system has asked us to save our current state.  Do so.
            //engine->app->savedState = malloc(sizeof(struct saved_state));
            //*((struct saved_state*)engine->app->savedState) = engine->state;
            //engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            LOG_MESSAGE << "engineHandleCommand called APP_CMD_INIT_WINDOW";

            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                ::engineInitDisplay(engine);
                //engine_draw_frame(engine);
            }
            
            DT3::GameMainThread::show_engine(engine->width, engine->height);
						
            break;
        case APP_CMD_TERM_WINDOW:
            LOG_MESSAGE << "engineHandleCommand called APP_CMD_TERM_WINDOW";

            // The window is being hidden or closed, clean it up.

             DT3::GameMainThread::hide_engine();

            ::engineTermDisplay(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            LOG_MESSAGE << "engineHandleCommand called APP_CMD_GAINED_FOCUS";

            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue, engine->accelerometerSensor);
                ASensorEventQueue_setEventRate(engine->sensorEventQueue, engine->accelerometerSensor, (1000L/60)*1000);
            }
            if (engine->magnetometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue, engine->magnetometerSensor);
                ASensorEventQueue_setEventRate(engine->sensorEventQueue, engine->magnetometerSensor, (1000L/60)*1000);
            }
            if (engine->gyroSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue, engine->gyroSensor);
                ASensorEventQueue_setEventRate(engine->sensorEventQueue, engine->gyroSensor, (1000L/60)*1000);
            }
            
            // Also start animating.
            engine->animating = 1;

			//DT3::GameMainThread::start(); // Note: Already running in another thread

            break;
        case APP_CMD_LOST_FOCUS:
            LOG_MESSAGE << "engineHandleCommand called APP_CMD_LOST_FOCUS";

            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue, engine->accelerometerSensor);
            }
            if (engine->magnetometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue, engine->magnetometerSensor);
            }
            if (engine->gyroSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue, engine->gyroSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            //engine_draw_frame(engine);
            
            //DT3::GameMainThread::stop(); // Note: Already running in another thread

            break;
        default:
            LOG_MESSAGE << "engineHandleCommand called Unknown";

    }
}

//==============================================================================
// This is the main entry point of a native application that is using
// android_native_app_glue.  It runs in its own thread, with its own
// event loop for receiving input events and doing other things.
//==============================================================================

void android_main(struct android_app* state) {
    LOG_MESSAGE << "android_main called";

    // Make sure glue isn't stripped. http://blog.beuc.net/posts/Make_sure_glue_isn__39__t_stripped/
    app_dummy();
    
    // Setup HAL
    HAL::initialize(state->activity->vm, state->activity->clazz);

    ::memset(&gEngine, 0, sizeof(Engine));
    state->userData = &gEngine;
    state->onAppCmd = engineHandleCommand;
    state->onInputEvent = engineHandleInput;
    gEngine.app = state;

    // Prepare to monitor accelerometer
    gEngine.sensorManager = ASensorManager_getInstance();
    gEngine.accelerometerSensor = ASensorManager_getDefaultSensor(gEngine.sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    gEngine.magnetometerSensor = ASensorManager_getDefaultSensor(gEngine.sensorManager, ASENSOR_TYPE_MAGNETIC_FIELD);
    gEngine.gyroSensor = ASensorManager_getDefaultSensor(gEngine.sensorManager, ASENSOR_TYPE_GYROSCOPE);
    gEngine.sensorEventQueue = ASensorManager_createEventQueue(gEngine.sensorManager, state->looper, LOOPER_ID_USER, NULL, NULL);

    //if (state->savedState != NULL) {
    //    // We are starting with a previous saved state; restore from it.
    //    engine.state = *(struct saved_state*)state->savedState;
    //}
    
    // Setup the file open callback. Every time a file gets opened, this callback
    // will intercept and redirect to an asset if necessary.
    FileManager::set_file_open_cb(make_callback(fileOpenCB), (void*) state);
    
    // Register callbacks
    SystemCallbacks::screen_inited_cb().add(make_callback(screen_inited));
    SystemCallbacks::screen_closed_cb().add(make_callback(screen_closed));
    SystemCallbacks::screen_swap_cb().add(make_callback(screen_swap));
    SystemCallbacks::screen_changed_cb().add(make_callback(screen_changed));
    
    // Scan the asset directory
    assetScan(state->activity->assetManager);

    DT3::GameMainThread::initialize_engine();

    // Initialize Analytics
    Analytics::record_event ("AppEvent", "AppEvent", "Started");

    // Loop waiting for stuff to do.
    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident = ALooper_pollAll(gEngine.animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {

                ASensorEvent event;
				while (ASensorEventQueue_getEvents(gEngine.sensorEventQueue, &event, 1) > 0) {
					//LOGI("accelerometer: x=%f y=%f z=%f", event.acceleration.x, event.acceleration.y, event.acceleration.z);
					
					switch (event.type) {
						case ASENSOR_TYPE_ACCELEROMETER:	
							DT3::GameMainThread::do_accelerometer(DT3::Vector3(event.acceleration.x, event.acceleration.y, event.acceleration.z) / ASENSOR_STANDARD_GRAVITY);
							break;
						case ASENSOR_TYPE_MAGNETIC_FIELD:
							DT3::GameMainThread::do_magnetometer(DT3::Vector3(event.magnetic.x, event.magnetic.y, event.magnetic.z));
							break;
						case ASENSOR_TYPE_GYROSCOPE:
							DT3::GameMainThread::do_gyro(DT3::Vector3(event.acceleration.x, event.acceleration.y, event.acceleration.z));
							break;
					}
					
				}
            	
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engineTermDisplay(&gEngine);
                DT3::GameMainThread::destroy_engine();
                return;
            }
        }

        if (gEngine.animating) {
            DT3::GameMainThread::loop();
        }
    }
}

//==============================================================================
//==============================================================================
