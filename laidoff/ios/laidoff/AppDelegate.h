//
//  AppDelegate.h
//  laidoff
//
//  Created by 김거엽 on 2017. 4. 15..
//  Copyright © 2017년 KIMGEO YEOB. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "laidoff.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;

- (void)setContext:(LWCONTEXT*) pLwc;

@end

