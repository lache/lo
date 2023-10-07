#import <Foundation/Foundation.h>
#import <CoreGraphics/CGImage.h>
#import <ImageIO/CGImageSource.h>
#import "lwbitmapcontext.h"

void test_print_main_bundle_path(const char* filename) {
    NSString *path = [NSString stringWithFormat:@"%@/%s",
                      [[NSBundle mainBundle] resourcePath],
                      filename];
    NSLog(@"mainBundle resource path: %@", path);
}

char* get_assets_path(const char* filename) {
    // DEV PATH: @"/Users/gb/laidoff/assets/"
    NSString *path = [NSString stringWithFormat:@"%@/%s",
                      [[NSBundle mainBundle] resourcePath],
                      filename];
    return strdup([path UTF8String]);
}

CGImageRef CGImageRef_load(const char *filename) {
    /*
    NSString *path = [NSString stringWithFormat:@"%@/%s",
                      [[NSBundle mainBundle] resourcePath],
                      filename];
     */
    NSString *path = [NSString stringWithFormat:@"%s", filename];
    NSImage *someImage = [[NSImage alloc] initWithContentsOfFile:path];
    
    
    if (someImage) {
        // create the image somehow, load from file, draw into it...
        CGImageSourceRef source;
        
        source = CGImageSourceCreateWithData((CFDataRef)[someImage TIFFRepresentation], NULL);
        CGImageRef maskRef =  CGImageSourceCreateImageAtIndex(source, 0, NULL);
        return maskRef;
        
    }
    return NULL;
}

unsigned char* CGImageRef_data(CGImageRef image, int* w, int* h) {
    NSInteger width = CGImageGetWidth(image);
    NSInteger height = CGImageGetHeight(image);
    unsigned char *data = (unsigned char*)calloc(1, width*height*4);
    
    CGContextRef context = CGBitmapContextCreate(data,
                                                 width, height,
                                                 8, width * 4,
                                                 CGImageGetColorSpace(image),
                                                 kCGImageAlphaPremultipliedLast);
    
    CGContextDrawImage(context,
                       CGRectMake(0.0, 0.0, (float)width, (float)height),
                       image);
    CGContextRelease(context);
    
    *w = width;
    *h = height;
    
    return data;
}

const unsigned char* load_png_osx(const char* filename, LWBITMAPCONTEXT* pBitmapContext) {
    
    CGImageRef r = CGImageRef_load(filename);
    const unsigned char* d = 0;
    if (r) {
        d = CGImageRef_data(r, &pBitmapContext->width, &pBitmapContext->height);
    }
    
    pBitmapContext->lock = 0;
    pBitmapContext->data = d;
    
    return d;
}

void unload_png_osx(LWBITMAPCONTEXT* pBitmapContext) {
    free(pBitmapContext->data);
    pBitmapContext->data = 0;
}

