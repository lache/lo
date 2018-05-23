#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "sound.h"

static NSSound* sound_pool[LWS_COUNT];
static NSString* sound_path[] = {
    @"assets/mp3/collapse",
    @"assets/mp3/collision",
    @"assets/mp3/damage",
    @"assets/mp3/dash1",
    @"assets/mp3/dash2",
    @"assets/mp3/defeat",
    @"assets/mp3/introbgm",
    @"assets/mp3/victory",
    @"assets/mp3/swoosh",
    @"assets/mp3/click",
    @"assets/mp3/ready",
    @"assets/mp3/steady",
    @"assets/mp3/go",
};

void preload_all_sound_osx() {
    for (int i = 0; i < LWS_COUNT; i++) {
        sound_pool[i] = [[NSSound alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:sound_path[i] ofType:@"mp3"] byReference:YES];
    }
}

void unload_all_sound_osx() {
    for (int i = 0; i < LWS_COUNT; i++) {
        [sound_pool[i] release];
    }
}

void play_sound_osx(LW_SOUND lws) {
    [sound_pool[lws] play];
    /*
    NSString *resourcePath = [[NSBundle mainBundle] pathForResource:@"assets/mp3/collision" ofType:@"mp3"];
    NSSound *sound = [[NSSound alloc] initWithContentsOfFile:resourcePath byReference:YES];
    [sound play];
     */
    //[sound release];
}
