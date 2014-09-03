//==============================================================================
///	
///	File: AppDelegate.m
///
/// Copyright (C) 2000-2014 by Smells Like Donkey Software Inc. All rights reserved.
///
/// This file is subject to the terms and conditions defined in
/// file 'LICENSE.txt', which is part of this source code package.
///	
//==============================================================================

#include "AppDelegate.h"
#include "DT3Core/Types/Utility/TouchEvent.hpp"
#include "DT3Core/Entry/GameMainThread.hpp"
#include "DT3Core/System/System.hpp"
#include "DT3Core/System/Globals.hpp"
#include "DT3Core/System/SystemCallbacks.hpp"
#include "DT3Core/Types/Utility/ConsoleStream.hpp"
#include "DT3Core/Types/Utility/Analytics.hpp"
#include "DT3Core/Types/Math/Matrix3.hpp"

#include "DT3Core/Devices/DeviceInput.hpp"
#include "DT3Core/Devices/DeviceMusic.hpp"
#include "DT3Core/Devices/DeviceAudio.hpp"

#include <sys/utsname.h>

#import <Social/Social.h>
#import <AVFoundation/AVFoundation.h>

#if DT3_IADS_ENABLED
    #import <iAd/iAd.h>
#endif

using namespace DT3;

//==============================================================================
//==============================================================================

AppViewController*	gController;
UIWindow*			gWindow;
EAGLView*			gGLView;

#if DT3_IADS_ENABLED
ADBannerView*       giAd = nil;
#endif

CGRect				gRect;
float               gScale = 1.0F;

UITouch*			gTouches[TouchEvent::MAX_NUM_TOUCHES] = { NULL };
TouchEvent          gTouchEvent;
    
//==============================================================================
// Define hooks from the engine
//==============================================================================

void screen_inited (void)
{
    // In order to render with OpenGL from another thread, this has
    // to be called in the thread. Conveniently, this callback will
    // be called when the screen has been opened.
    [gGLView makeCurrentContext];
}

void screen_closed (void)
{
    // Screen is invalid so clear the current context
    [gGLView clearCurrentContext];
}

void screen_changed (DTuint width, DTuint height)
{
    // In order to render with OpenGL from another thread, this has
    // to be called in the thread. Conveniently, this callback will
    // be called when the screen has been opened.
    [gGLView makeCurrentContext];
}

void screen_swap (void)
{
    [gGLView swapBuffers];
}

//==============================================================================
// Social media routines
//==============================================================================

void show_twitter (const std::string &msg)
{
    SLComposeViewController *tweetSheet = [SLComposeViewController composeViewControllerForServiceType:SLServiceTypeTwitter];
    
    NSString* ns_msg = [NSString stringWithUTF8String:msg.c_str()];
    
    [tweetSheet setInitialText:ns_msg];
    [gController presentViewController:tweetSheet animated:YES completion:nil];
}

void show_facebook (const std::string &msg)
{
    SLComposeViewController *tweetSheet = [SLComposeViewController composeViewControllerForServiceType:SLServiceTypeFacebook];
    
    NSString* ns_msg = [NSString stringWithUTF8String:msg.c_str()];
    
    [tweetSheet setInitialText:ns_msg];
    [gController presentViewController:tweetSheet animated:YES completion:nil];
}

//==============================================================================
// Ads
//==============================================================================

void show_ads (void)
{
#if DT3_IADS_ENABLED
    [[NSOperationQueue mainQueue] addOperationWithBlock:^ {

        if (!giAd) {
            giAd = [[ADBannerView alloc] initWithFrame:CGRectZero];

            CGRect banner_frame = CGRectZero;
            banner_frame.size = [giAd sizeThatFits:gGLView.frame.size];

            if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
                banner_frame.origin.x = (gRect.size.width - banner_frame.size.width) / 2;
            } else {
                banner_frame.origin.x = (gRect.size.height - banner_frame.size.width) / 2;
            }
            
            giAd.frame = banner_frame;
            
            [giAd setHidden:NO];
            [giAd setAlpha:0.0F];

            [gGLView addSubview:giAd];
            
            [UIView beginAnimations:nil context:NULL];
            [UIView setAnimationDuration:2.0];
            [giAd setAlpha:1.0];
            [UIView commitAnimations];
        }

    }];

#endif
}

void hide_ads (void)
{
#if DT3_IADS_ENABLED
    [[NSOperationQueue mainQueue] addOperationWithBlock:^ {
    
        if (giAd) {
            [giAd removeFromSuperview];
            [giAd release];
            giAd = nil;
        }
        
    }];
#endif
}

//==============================================================================
// View Controller for app
//==============================================================================

@implementation AppViewController

- (id)init
{
    self = [super init];
    if (self) {		
		self.view = gGLView;
		self.view.autoresizingMask = (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
		self.view.clipsToBounds = YES;
                
        [self setWantsFullScreenLayout:YES];
    }
    
    return self;
}

- (void)dealloc {	
	[super dealloc];
}

//==============================================================================
//==============================================================================

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {

        switch (interfaceOrientation) {
            case UIInterfaceOrientationPortrait:
            case UIInterfaceOrientationPortraitUpsideDown:
                return YES;
                
            default:
                return NO;
        
        }

    } else {

        switch (interfaceOrientation) {
            case UIInterfaceOrientationLandscapeLeft:
            case UIInterfaceOrientationLandscapeRight:
                return YES;
                
            default:
                return NO;
        
        }


    }

}

-(void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    if (!System::input_manager())
        return;
    
    switch (toInterfaceOrientation) {
        case UIInterfaceOrientationLandscapeLeft:   // Home button on left
            break;
            
        case UIInterfaceOrientationLandscapeRight:  // Home button on right
            break;

        case UIInterfaceOrientationPortrait:
            break;
            
        case UIInterfaceOrientationPortraitUpsideDown:
            break;
            
        default:
            break;
    }
    
}

//==============================================================================
//==============================================================================

- (NSUInteger)supportedInterfaceOrientations {
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
    } else {
        return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
    }
}

- (BOOL) shouldAutorotate {
    return YES;
}

@end

//==============================================================================
// App delegate
//==============================================================================

@implementation AppDelegate

- (id)init
{
    self = [super init];
    if (self) {		
        motionManager = [[CMMotionManager alloc] init];
        motionQueue = [[NSOperationQueue alloc] init];
    }
    
    return self;
}

- (void)dealloc {	
	[super dealloc];
    
    [motionQueue release];
    [motionManager release];
}

- (void) touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{	

	NSArray *all_touches = [touches allObjects];
	
    int touch_count = [all_touches count];
	for (int i = 0; i < touch_count; ++i) {
		UITouch*	touch = [all_touches objectAtIndex:i];
		CGPoint		location = [touch locationInView:gGLView];
		
		// Figure out index of touches
		for (int j = 0; j < TouchEvent::MAX_NUM_TOUCHES; ++j) {
			if (gTouches[j] == NULL) {
                LOG_MESSAGE << "touchesBegan " << j;
            
				gTouches[j] = touch;
                
                gTouchEvent.touches[j].state = TouchEvent::STATE_PRESSED;
                gTouchEvent.touches[j].pos = Vector2(location.x*gScale,location.y*gScale);
                gTouchEvent.touches[j].previous_pos = Vector2(location.x*gScale,location.y*gScale);
                gTouchEvent.touches[j].first_pos = Vector2(location.x*gScale,location.y*gScale);
                gTouchEvent.touches[j].delta = Vector2(0.0F,0.0F);
                gTouchEvent.touches[j].dt = 0.0F;
                gTouchEvent.touches[j].velocity = Vector2(0.0F,0.0F);
                
                gTouchEvent.touches[j].timer.delta_time();
   
                GameMainThread::touch_event(gTouchEvent);

				break;
			}
		}
	}
	
}

- (void) touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	NSArray *all_touches = [touches allObjects];
	
    int touch_count = [all_touches count];
	for (int i = 0; i < touch_count; ++i) {
		UITouch*	touch = [all_touches objectAtIndex:i];
		CGPoint		location = [touch locationInView:gGLView];

		// Figure out index of touches
		for (int j = 0; j < TouchEvent::MAX_NUM_TOUCHES; ++j) {
			if (gTouches[j] == touch) {
                LOG_MESSAGE << "touchesMoved " << j;
                
                gTouchEvent.touches[j].state = TouchEvent::STATE_DOWN;
                gTouchEvent.touches[j].previous_pos = gTouchEvent.touches[j].pos;
                gTouchEvent.touches[j].pos = Vector2(location.x*gScale,location.y*gScale);
                gTouchEvent.touches[j].delta = gTouchEvent.touches[j].pos - gTouchEvent.touches[j].previous_pos;
                gTouchEvent.touches[j].dt = gTouchEvent.touches[j].timer.delta_time();
                gTouchEvent.touches[j].velocity = gTouchEvent.touches[j].delta / gTouchEvent.touches[j].dt;

                GameMainThread::touch_event(gTouchEvent);

				break;
			}
		}
	}
}

- (void) touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	NSArray *all_touches = [touches allObjects];

    int touch_count = [all_touches count];
	for (int i = 0; i < touch_count; ++i) {
		UITouch*	touch = [all_touches objectAtIndex:i];
		CGPoint		location = [touch locationInView:gGLView];

		// Figure out index of touches
		for (int j = 0; j < TouchEvent::MAX_NUM_TOUCHES; ++j) {
			if (gTouches[j] == touch) {
                LOG_MESSAGE << "touchesEnded " << j;

				gTouches[j] = NULL;
				
                gTouchEvent.touches[j].state = TouchEvent::STATE_RELEASED;
                gTouchEvent.touches[j].previous_pos = gTouchEvent.touches[j].pos;
                gTouchEvent.touches[j].pos = Vector2(location.x*gScale,location.y*gScale);
                gTouchEvent.touches[j].delta = gTouchEvent.touches[j].pos - gTouchEvent.touches[j].previous_pos;
                gTouchEvent.touches[j].dt = gTouchEvent.touches[j].timer.delta_time();
                gTouchEvent.touches[j].velocity = gTouchEvent.touches[j].delta / gTouchEvent.touches[j].dt;

                GameMainThread::touch_event(gTouchEvent);

                gTouchEvent.touches[j].state = TouchEvent::STATE_NONE;

				break;
			}
		}
	}
}

//==============================================================================
// Rotates the sensor data so that it matches the UI orientation. ie.
// +Y will always be "up" according to the UI.
//==============================================================================

- (void) rotateSensorData:(Vector3*)sensor toOrientation:(UIInterfaceOrientation) orientation
{
    Matrix3 rot(Matrix3::identity());

    switch (orientation) {
        case UIInterfaceOrientationLandscapeLeft:   // Home button on left
            rot = Matrix3 ( 0.0F,   1.0F,   0.0F,
                            -1.0F,  0.0F,  0.0F,
                            0.0F,   0.0F,   1.0F );
            break;
            
        case UIInterfaceOrientationLandscapeRight:   // Home button on right
            rot = Matrix3 ( 0.0F,   -1.0F,   0.0F,
                            1.0F,   0.0F,  0.0F,
                            0.0F,   0.0F,   1.0F );
            break;

            
        case UIInterfaceOrientationPortraitUpsideDown:
            rot = Matrix3 ( -1.0F,  0.0F,   0.0F,
                            0.0F,   -1.0F,  0.0F,
                            0.0F,   0.0F,   1.0F );
            break;
            
        case UIInterfaceOrientationPortrait:
            rot = Matrix3 ( 1.0F,   0.0F,   0.0F,
                            0.0F,   1.0F,   0.0F,
                            0.0F,   0.0F,   1.0F );
            break;
    }

    *sensor = rot * (*sensor);
}

//==============================================================================
//==============================================================================

-(BOOL)startAppWithOptions:(NSDictionary*)launchOptions application:(UIApplication*)application
{
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];  
    
    //
    // Start engine
    //

    // Initialize the engine. Do it before "start" is called on the main loop. This will run it synchronously
    GameMainThread::initialize_engine();
    
    // Register callbacks
    SystemCallbacks::screen_inited_cb().add(make_callback(screen_inited));
    SystemCallbacks::screen_closed_cb().add(make_callback(screen_closed));
    SystemCallbacks::screen_swap_cb().add(make_callback(screen_swap));
    SystemCallbacks::screen_changed_cb().add(make_callback(screen_changed));

    //
    // Initialize Analytics
    //
    
    // Pirated Flag
//	FilePath piracy_check("{APPDIR}/_CodeSignature/CodeResources");
//    DTboolean pirated = !piracy_check.exists();
    
    // Initialize Analytics
    Analytics::record_event ("AppEvent", "AppEvent", "Started");

    //
    // Build Screen
    //
    
    // Small Screen
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        Globals::set_global("SYS_SMALL_SCREEN", "1", Globals::VOLATILE);
    } else {
        Globals::set_global("SYS_SMALL_SCREEN", "0", Globals::VOLATILE);
    }

    // Change resolution for retina
    if([[UIScreen mainScreen] respondsToSelector:@selector(scale)])
        gScale = [UIScreen mainScreen].scale;

//#if DT3_RUN_IPHONE4_AT_HALF_RES
//    // The iPhone 4 is a pretty fill rate limited device so lets turn off Retina support.
//
//    struct utsname systemInfo;
//    ::uname(&systemInfo);
//    
//    if (::strcmp(systemInfo.machine, "iPhone3,1") == 0)
//        gScale = 1.0F;
//#endif

	gRect = [UIScreen mainScreen].applicationFrame;
    CGRect gRectTransposed = CGRectMake(0, 0, gRect.size.height, gRect.size.width);
        
    //
    // Build gl view
    //

    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        gGLView = [[EAGLView alloc] initWithFrame:gRect];
    } else {
        gGLView = [[EAGLView alloc] initWithFrame:gRectTransposed];
    }

    if (gScale != 1.0F)
        gGLView.contentScaleFactor = gScale;
        
    //
    // Build window hierarchy
    //
    
	gController = [[AppViewController alloc] init];
	gWindow = [[UIWindow alloc] initWithFrame:gRect];
    gWindow.rootViewController = gController; 

	[gWindow addSubview:gGLView];
    [gWindow makeKeyAndVisible];
            
	return YES;
}

//2.0 - Application Launching Method
- (void)applicationDidFinishLaunching:(UIApplication*)application
{
	[self startAppWithOptions:nil application:application];
}

//3.0 - Application Launching Method
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions: (NSDictionary*)launchOptions
{
	return [self startAppWithOptions:launchOptions application:application];
}

//==============================================================================
//==============================================================================

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Clear touches
    for (int j = 0; j < TouchEvent::MAX_NUM_TOUCHES; ++j)
        gTouches[j] = NULL;
    gTouchEvent.clear();

    // Analytics
    Analytics::record_event ("AppEvent", "AppEvent", "Resumed");
    
    //
    // Startup audio
    //
    
	if (System::music_renderer())	System::music_renderer()->restart_audio();
	if (System::audio_renderer())	System::audio_renderer()->restart_audio();

    //
    // Start Main Loop
    //
    
    GameMainThread::start();

    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        GameMainThread::show_engine(gRect.size.width*gScale, gRect.size.height*gScale);
    } else {
        GameMainThread::show_engine(gRect.size.height*gScale, gRect.size.width*gScale);
    }
    
    
    //
    // Motion manager
    // 
    
    motionManager.accelerometerUpdateInterval = 1.0F/100.0F;
    [motionManager startAccelerometerUpdatesToQueue: motionQueue withHandler: 
        ^( CMAccelerometerData* data, NSError* error) {
            Vector3 s(data.acceleration.x, data.acceleration.y, data.acceleration.z);
            [self rotateSensorData:&s toOrientation:gController.interfaceOrientation];
            GameMainThread::do_accelerometer(s);
        }
    ];

    motionManager.gyroUpdateInterval = 1.0F/100.0F;
    [motionManager startGyroUpdatesToQueue: motionQueue withHandler: 
        ^( CMGyroData* data, NSError* error) {
            Vector3 s(data.rotationRate.x, data.rotationRate.y, data.rotationRate.z);
            [self rotateSensorData:&s toOrientation:gController.interfaceOrientation];
            GameMainThread::do_gyro(s);
        }
    ];

    motionManager.magnetometerUpdateInterval = 1.0F/100.0F;
    [motionManager startMagnetometerUpdatesToQueue: motionQueue withHandler:
        ^( CMMagnetometerData* data, NSError* error) {
            Vector3 s(data.magneticField.x, data.magneticField.y, data.magneticField.z);
            [self rotateSensorData:&s toOrientation:gController.interfaceOrientation];
            GameMainThread::do_magnetometer(s);
        }
    ];

}

//==============================================================================
//==============================================================================

- (void) applicationWillResignActive:(UIApplication*)application
{

    GameMainThread::stop();

    //
    // Turn off audio
    //

	if (System::music_renderer())	System::music_renderer()->shutdown_audio();
	if (System::audio_renderer())	System::audio_renderer()->shutdown_audio();

    [motionManager stopAccelerometerUpdates];
    [motionManager stopGyroUpdates];
    [motionManager stopMagnetometerUpdates];

    Analytics::record_event ("AppEvent", "AppEvent", "Suspended");

    //
    // Hide the engine
    //

    GameMainThread::hide_engine();
}
//==============================================================================
//==============================================================================

- (void) applicationWillTerminate:(UIApplication*)application
{     
    
    // Analytics
    Analytics::record_event ("AppEvent", "AppEvent", "Stopped");
    
    //
    // Destroy the engine
    //
    
    GameMainThread::destroy_engine();
}

//==============================================================================
//==============================================================================

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	NSLog(@"**************************************************************");
	NSLog(@"*                                                            *");
	NSLog(@"*                   Low memory warning!!!                    *");
	NSLog(@"*                                                            *");
	NSLog(@"**************************************************************");
    

}

//==============================================================================
//==============================================================================

@end
