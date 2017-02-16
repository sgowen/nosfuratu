//
//  GameViewController.m
//  nosfuratu
//
//  Created by Stephen Gowen on 9/5/14.
//  Copyright (c) 2016 Noctis Games. All rights reserved.
//

#import "GameViewController.h"

#import "UIView+Toast.h"
#import "GameScreenController.h"

#import "GoogleMobileAds/GoogleMobileAds.h"

// C++
#include "IOSOpenGLGameScreen.h"
#include "ScreenInputManager.h"

@interface GameViewController () <GADInterstitialDelegate>
{
    IOSOpenGLGameScreen *gameScreen;
    GameScreenController *_gameScreenController;
}

@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GADInterstitial *interstitial;


@end

@implementation GameViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    if (!self.context)
    {
        NSLog(@"Failed to create ES context");
        return;
    }
    
    GLKView *view = (GLKView *)self.view;
    
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    view.userInteractionEnabled = YES;
    [view setMultipleTouchEnabled:YES];
    
    self.preferredFramesPerSecond = 60;
    
    [EAGLContext setCurrentContext:self.context];
    
    CGRect screenBounds = [[UIScreen mainScreen] nativeBounds];
    
    CGSize size = CGSizeMake(screenBounds.size.width, screenBounds.size.height);
    size.width = roundf(size.width);
    size.height = roundf(size.height);
    
    NSLog(@"dimension %f x %f", size.width, size.height);
    
    [view bindDrawable];
    
    unsigned long long ramSize = [NSProcessInfo processInfo].physicalMemory;
    bool isLowMemoryDevice = ramSize < 1610612736; // 1536 MB
    
    NSLog(@"ramSize: %llu", ramSize);
    NSLog(@"isLowMemoryDevice: %@", isLowMemoryDevice ? @"YES" : @"NO");
    
    gameScreen = new IOSOpenGLGameScreen(MAX(size.width, size.height), MIN(size.width, size.height), [UIScreen mainScreen].applicationFrame.size.width, [UIScreen mainScreen].applicationFrame.size.height, isLowMemoryDevice);
    
    _gameScreenController = [[GameScreenController alloc] initWithGameScreen:gameScreen getLevelFilePath:^NSString *(NSString *levelFileName)
    {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentsDirectory = [paths objectAtIndex:0];
        NSString *filePath = [documentsDirectory stringByAppendingPathComponent:levelFileName];
        
        return filePath;
    } displayMessageBlock:^(NSString *message)
    {
        [self.view makeToast:message];
    } andHandleInterstitialAd:^
    {
        if (self.interstitial.isReady)
        {
            [self.interstitial presentFromRootViewController:self];
        }
    }];
    
    [self createAndLoadInterstitial];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onPause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(onResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    gameScreen->onResume();
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    
    [self onPause];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch *touch = [touches anyObject];
    CGPoint pos = [touch locationInView: [UIApplication sharedApplication].keyWindow];
    SCREEN_INPUT_MANAGER->onTouch(ScreenEventType_DOWN, pos.x, pos.y);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch *touch = [touches anyObject];
    CGPoint pos = [touch locationInView: [UIApplication sharedApplication].keyWindow];
    SCREEN_INPUT_MANAGER->onTouch(ScreenEventType_DRAGGED, pos.x, pos.y);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    UITouch *touch = [touches anyObject];
    CGPoint pos = [touch locationInView: [UIApplication sharedApplication].keyWindow];
    SCREEN_INPUT_MANAGER->onTouch(ScreenEventType_UP, pos.x, pos.y);
}

#pragma mark <GLKViewControllerDelegate>

- (void)update
{
    [_gameScreenController update:self.timeSinceLastUpdate];
}

#pragma mark <GLKViewDelegate>

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    [_gameScreenController present];
}

#pragma mark Private

- (void)onResume
{
    [_gameScreenController resume];
}

- (void)onPause
{
    [_gameScreenController pause];
}

- (void)createAndLoadInterstitial
{
#if DEBUG
    self.interstitial = [[GADInterstitial alloc] initWithAdUnitID:@"ca-app-pub-3940256099942544/4411468910"];
#else
    self.interstitial = [[GADInterstitial alloc] initWithAdUnitID:@"ca-app-pub-6017554042572989/7161041758"];
#endif
    
    self.interstitial.delegate = self;
    [self.interstitial loadRequest:[GADRequest request]];
}

- (void)interstitialDidDismissScreen:(GADInterstitial *)interstitial
{
    [self createAndLoadInterstitial];
}

@end
