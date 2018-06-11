//
//  AppDelegate.m
//  laidoff
//
//  Created by 김거엽 on 2017. 4. 15..
//  Copyright © 2017년 KIMGEO YEOB. All rights reserved.
//

#import "AppDelegate.h"
#include "lwime.h"

void set_app_delegate(id ad);
void lw_set_push_token(LWCONTEXT* pLwc, int domain, const char* token);

@interface AppDelegate ()

@property (nonatomic) LWCONTEXT *pLwc;

@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    set_app_delegate(self);
    // latest version check
    NSString* bundleVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    NSLog(@"laidoff bundleVersion: %@", bundleVersion);
    if ([bundleVersion isEqual: @"0.1.0"]) {
        NSLog(@"laidoff bundleVersion - debug version");
        // no latest version check
    } else if ([bundleVersion hasPrefix:@"1."]) {
        NSLog(@"laidoff bundleVersion - appstore version");
        // no latest version check
    } else {
        NSLog(@"laidoff bundleVersion - adhoc version");
        // check if there is a latest version on internet...
        NSString *stringURL = @"https://s3.ap-northeast-2.amazonaws.com/sky.popsongremix.com/laidoff/ipa/versionName.txt";
        NSURL  *url = [NSURL URLWithString:stringURL];
        NSData *urlData = [NSData dataWithContentsOfURL:url];
        if ( urlData ) {
            NSString *versionName = [[[NSString alloc] initWithData:urlData encoding:NSUTF8StringEncoding] stringByTrimmingCharactersInSet:[NSCharacterSet newlineCharacterSet]];
            NSLog(@"latest version on internet: %@", versionName);
            if ([versionName isEqual: bundleVersion]) {
                // you have up-to-date version
            } else {
                // open browser
                NSString *installUrlStr = [NSString stringWithFormat:@"https://s3.ap-northeast-2.amazonaws.com/sky.popsongremix.com/laidoff/ipa/install.html?currentVersion=%@&latestVersion=%@", bundleVersion, versionName];
                NSURL *installUrl = [NSURL URLWithString:installUrlStr];
                [[UIApplication sharedApplication] openURL:installUrl];
            }
        } else {
            // error while checking latest version
        }
    }
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter]
     addObserver:self selector:@selector(orientationChanged:)
     name:UIDeviceOrientationDidChangeNotification
     object:[UIDevice currentDevice]];
    return YES;
}

- (void)orientationChanged:(NSNotification *)note
{
    CGRect screenRect = [[UIScreen mainScreen] bounds];
    CGFloat screenScale = [[UIScreen mainScreen] nativeScale];
    CGFloat screenWidth = screenScale * screenRect.size.width;
    CGFloat screenHeight = screenScale * screenRect.size.height;
    lw_set_viewport_size(self.pLwc, (int)screenWidth, (int)screenHeight);
    lw_set_window_size(self.pLwc, (int)screenWidth, (int)screenHeight);
    
    UIDevice * device = note.object;
    switch(device.orientation)
    {
        case UIDeviceOrientationPortrait:
            break;
        case UIDeviceOrientationPortraitUpsideDown:
            break;
        case UIDeviceOrientationFaceUp:
            break;
        case UIDeviceOrientationUnknown:
            break;
        case UIDeviceOrientationFaceDown:
            break;
        case UIDeviceOrientationLandscapeLeft:
            break;
        case UIDeviceOrientationLandscapeRight:
            break;
        default:
            break;
    };
}

- (void)application:(UIApplication *)application
didRegisterUserNotificationSettings:(UIUserNotificationSettings *)notificationSettings
{
    //[application registerForRemoteNotifications];
}

- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken
{
    //NSLog(@"deviceToken: %@", deviceToken);
    NSString * token = [NSString stringWithFormat:@"%@", deviceToken];
    //Format token as you need:
    token = [token stringByReplacingOccurrencesOfString:@" " withString:@""];
    token = [token stringByReplacingOccurrencesOfString:@">" withString:@""];
    token = [token stringByReplacingOccurrencesOfString:@"<" withString:@""];
    NSLog(@"deviceToken: %@", token);
    const char* tokenCstr = [token UTF8String];
    lw_set_push_token(self.pLwc, 1, tokenCstr);
}

- (void)application:(UIApplication *)app didFailToRegisterForRemoteNotificationsWithError:(NSError *)err {
    NSString *str = [NSString stringWithFormat: @"Error: %@", err];
    NSLog(@"%@",str);
}

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo  {
    application.applicationIconBadgeNumber = 0;
    //self.textView.text = [userInfo description];
    // We can determine whether an application is launched as a result of the user tapping the action
    // button or whether the notification was delivered to the already-running application by examining
    // the application state.
    
    if (application.applicationState == UIApplicationStateActive) {
        // Nothing to do if applicationState is Inactive, the iOS already displayed an alert view.
        UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:@"Check this out"
                                                            message:[NSString stringWithFormat:@"%@",[[userInfo objectForKey:@"aps"] objectForKey:@"alert"]]delegate:self cancelButtonTitle:@"Got it!" otherButtonTitles:nil];
        [alertView show];
    }
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}


- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    if (alertView.tag == 10000) {
        // 0 = Tapped yes
        if (buttonIndex == 1)
        {
            NSLog(@"Registering for push notifications...");
            [[UIApplication sharedApplication] registerUserNotificationSettings:[UIUserNotificationSettings settingsForTypes:(UIUserNotificationTypeSound | UIUserNotificationTypeAlert | UIUserNotificationTypeBadge) categories:nil]];
            [[UIApplication sharedApplication] registerForRemoteNotifications];
        }
    } else {
        NSString* textInput = [[alertView textFieldAtIndex:0] text];
        if (textInput && buttonIndex == 1) {
            NSLog(@"Entered: %@", textInput);
            strcpy(lw_get_text_input_for_writing(), (char*)[textInput UTF8String]);
            lw_increase_text_input_seq();
        }
    }
}

- (void)setContext:(LWCONTEXT*) pLwc {
    self.pLwc = pLwc;
}
@end
