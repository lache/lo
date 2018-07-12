//
//  GameViewController.h
//  laidoff
//
//  Created by 김거엽 on 2017. 4. 15..
//  Copyright © 2017년 KIMGEO YEOB. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

API_AVAILABLE(ios(9.0))
@interface GameViewController : GLKViewController
@property (weak, nonatomic) IBOutlet UIStackView *chatGroup;
@property (weak, nonatomic) IBOutlet UITextField *chatTextField;


@end
