//
//  GameViewController.m
//  laidoff
//
//  Created by 김거엽 on 2017. 4. 15..
//  Copyright © 2017년 KIMGEO YEOB. All rights reserved.
//

#import "GameViewController.h"
#import "lwgl.h"
#import "laidoff.h"
#import "input.h"
#import "AppDelegate.h"
#import "UIDeviceHardware.h"
#include "lwttl.h"

API_AVAILABLE(ios(9.0))
GameViewController *viewController;


@interface GameViewController () {
    
}
@property (strong, nonatomic) EAGLContext *context;
@property (nonatomic) LWCONTEXT *pLwc;
@property (nonatomic) UITapGestureRecognizer *tapRecognizer;
@property (nonatomic) BOOL keyboardShown;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation GameViewController

char internal_data_path[1024];

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat16;
    view.userInteractionEnabled = YES;
    view.multipleTouchEnabled = YES;
    viewController = self;
    self.preferredFramesPerSecond = 60;
    [self setupGL];
    
    CGRect screenRect = [[UIScreen mainScreen] bounds];
    CGFloat screenScale = [[UIScreen mainScreen] nativeScale];
    CGFloat screenWidth = screenScale * screenRect.size.width;
    CGFloat screenHeight = screenScale * screenRect.size.height;
    
    self.pLwc = lw_init_initial_size((int)screenWidth, (int)screenHeight);
    NSString* platformString = [UIDeviceHardware platformString];
    NSString* bundleVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    set_package_version([bundleVersion UTF8String]);
    lw_set_device_model(self.pLwc, [platformString UTF8String]);
	lw_set_viewport_size(self.pLwc, (int)screenWidth, (int)screenHeight);
    lw_set_window_size(self.pLwc, (int)screenWidth, (int)screenHeight);

    self.pLwc->internal_data_path = internal_data_path;
    self.pLwc->user_data_path = internal_data_path;
    strcpy(internal_data_path, getenv("HOME"));
    strcat(internal_data_path, "/Documents");
    NSLog(@"Internal data path: %s", internal_data_path);
    lwc_start_logic_thread(self.pLwc);
    
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];

    [appDelegate setContext:self.pLwc];
    
    self.tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(dismissKeyboard)];
    
    [viewController.chatGroup setHidden:YES];
}

-(void)dismissKeyboard
{
    [self.chatTextField resignFirstResponder];
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    
    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }
    
    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIKeyboardWillHideNotification object:nil];
}

#pragma mark - keyboard movements
- (void)keyboardWillShow:(NSNotification *)notification
{
    // move upward when keyboard shown
    CGSize keyboardSize = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
    [UIView animateWithDuration:0.3 animations:^{
        CGRect f = self.view.frame;
        f.origin.y = -keyboardSize.height;
        self.view.frame = f;
    }];
    // register tap recognizer
    [self.view addGestureRecognizer:self.tapRecognizer];
    self.keyboardShown = YES;
}

-(void)keyboardWillHide:(NSNotification *)notification
{
    // move downward when keyboard hidden
    [UIView animateWithDuration:0.3 animations:^{
        CGRect f = self.view.frame;
        f.origin.y = 0;
        self.view.frame = f;
    }];
    // remove tap recognizer
    [self.view removeGestureRecognizer:self.tapRecognizer];
    self.keyboardShown = NO;
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    //lwc_update(self.pLwc, self.timeSinceLastUpdate);
}

static CGPoint getNormalizedPoint(UIView* view, CGPoint locationInView)
{
    const float normalizedX = (locationInView.x / view.bounds.size.width) * 2.f - 1.f;
    const float normalizedY = -((locationInView.y / view.bounds.size.height) * 2.f - 1.f);
    return CGPointMake(normalizedX, normalizedY);
}

const int MAX_TOUCHES = 11;

void* g_touchTracker[MAX_TOUCHES];

int AddNewTouch(void *touch)
{
    for (int i = 0; i < MAX_TOUCHES; i++)
    {
        if (!g_touchTracker[i])
        {
            g_touchTracker[i] = touch;
            return i;
        }
    }
    return -1;
}

void RemoveTouch(void *touch)
{
    int trackId = GetFingerTrackId(touch);
    if (trackId >= 0 && trackId < MAX_TOUCHES)
    {
        g_touchTracker[trackId] = NULL;
    }
}

int GetFingerTrackId(void *touch)
{
    for (int i = 0; i < MAX_TOUCHES; i++)
    {
        if (g_touchTracker[i] == touch)
        {
            return i;
        }
    }
    return -1;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (self.keyboardShown) {
        return;
    }
    //[super touchesBegan:touches withEvent:event];
    for (UITouch *touchEvent in touches)
    {
        int fingerId = AddNewTouch((__bridge void*)touchEvent);
        CGPoint locationInView = [touchEvent locationInView:self.view];
        CGPoint normalizedPoint = getNormalizedPoint(self.view, locationInView);
        //on_touch_press(normalizedPoint.x, normalizedPoint.y);
        lw_trigger_touch(self.pLwc, normalizedPoint.x, normalizedPoint.y, fingerId);
        lw_trigger_mouse_press(self.pLwc, normalizedPoint.x, normalizedPoint.y, fingerId);
        //NSLog(@"BEGAN Touch count: %d", [[event allTouches]count]);
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (self.keyboardShown) {
        return;
    }
    //[super touchesMoved:touches withEvent:event];
    for (UITouch *touchEvent in touches)
    {
        int fingerId = GetFingerTrackId((__bridge void*)touchEvent);
        CGPoint locationInView = [touchEvent locationInView:self.view];
        CGPoint normalizedPoint = getNormalizedPoint(self.view, locationInView);
        //on_touch_drag(normalizedPoint.x, normalizedPoint.y);
        lw_trigger_mouse_move(self.pLwc, normalizedPoint.x, normalizedPoint.y, fingerId);
        //NSLog(@"MOVED Touch count: %d", [[event allTouches]count]);
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (self.keyboardShown) {
        return;
    }
    //[super touchesEnded:touches withEvent:event];
    for (UITouch *touchEvent in touches)
    {
        int fingerId = GetFingerTrackId((__bridge void*)touchEvent);
        CGPoint locationInView = [touchEvent locationInView:self.view];
        CGPoint normalizedPoint = getNormalizedPoint(self.view, locationInView);
        lw_trigger_mouse_release(self.pLwc, normalizedPoint.x, normalizedPoint.y, fingerId);
        //NSLog(@"ENDED Touch count: %d", [[event allTouches]count]);
        RemoveTouch((__bridge void*)touchEvent);
    }
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
    for (UITouch *touchEvent in touches)
    {
        RemoveTouch((__bridge void*)touchEvent);
    }

}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    lwc_prerender_mutable_context(self.pLwc);
    lwc_render(self.pLwc);
}

- (IBAction)sendChat:(id)sender {
    lwttl_udp_send_ttlchat(self.pLwc->ttl, lwttl_sea_udp(self.pLwc->ttl), [self.chatTextField.text UTF8String]);
    [self.chatTextField setText:@""];
}

-(BOOL) textFieldShouldReturn:(UITextField *)textField
{
    [self sendChat:textField];
    return YES;
}

void lw_open_chat(void) {
    if (@available(iOS 9.0, *)) {
        BOOL hidden = [viewController.chatGroup isHidden];
        [viewController.chatGroup setHidden:!hidden];
        if (hidden) {
            [viewController.chatTextField becomeFirstResponder];
            
        }
    } else {
        // Fallback on earlier versions
    }
    /*
    UIStoryboard *sb = [UIStoryboard storyboardWithName:@"ChatStoryboard" bundle:nil];
    UIViewController *vc = [sb instantiateInitialViewController];
    vc.modalTransitionStyle = UIModalTransitionStyleFlipHorizontal;
    [viewController presentViewController:vc animated:YES completion:NULL];
     */
}

@end
