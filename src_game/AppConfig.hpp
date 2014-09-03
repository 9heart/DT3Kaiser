#ifndef DT3_APPCONFIG
#define DT3_APPCONFIG
//==============================================================================
///	
///	File: AppConfig.hpp
///
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================


//==============================================================================
//==============================================================================

namespace DT3 {

//==============================================================================
//==============================================================================

#define PROJECT_NAME "kaiser"

// Clear graphics on suspend
#define DT3_CLEAR_GRAPHICS_ON_SUSPEND 1

// Enable fixed frame rate
#define DT3_UPDATE_RATE_PERIOD (1.0F/60.0F)     
#define DT3_UPDATE_MAX_SKIP_FRAMES 3
#define DT3_MAX_TICK 0.1F

// Analytics
#define DT3_USE_ANALYTICS 1

// Portal
#define DTP_USE_PORTAL 1
#define DTP_PUBLIC_ID "CHANGEME"
#define DTP_PRIVATE_KEY "CHANGEME"

// Debugging aids
#define DT3_GUARD_ALLOCATIONS 1

// Profiler
#if DT3_OS != DT3_ANDROID && DT3_OS != DT3_IOS
    #define DT3_USE_PROFILER 1
#endif

//==============================================================================
/// Class
//==============================================================================

class AppConfig {
    public:        
                                    AppConfig				(void);	
									AppConfig				(const AppConfig &rhs);
        AppConfig &                 operator =				(const AppConfig &rhs);	
        virtual                     ~AppConfig				(void);

    public:        
			
		/// The app version number for archining
		/// \return app archive version number
        /// Note: returns unsigned int so we don't have to include anything else
		static unsigned int			app_version     (void);
        
};

//==============================================================================
//==============================================================================

} // DT3

#endif


