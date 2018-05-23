#include <stdlib.h>
#include <string.h>

#include "platform_detection.h"

#if LW_PLATFORM_WIN32
#include <Wincodec.h>
#else
typedef int HRESULT;
#endif

#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
#include "glad/glad.h"
#endif

#include "lwbitmapcontext.h"
#include "etc1.h"
#include "file.h"
#include "lwcontext.h"
#include "lwpkm.h"

extern "C" unsigned short swap_bytes(unsigned short aData);
extern "C" unsigned int swap_4_bytes(unsigned int num);

#if LW_PLATFORM_OSX
extern "C" const unsigned char* load_png_osx(const char* filename, LWBITMAPCONTEXT* pBitmapContext);
extern "C" void unload_png_osx(LWBITMAPCONTEXT* pBitmapContext);
#elif LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR
extern "C" const unsigned char* load_png_ios(const char* filename, LWBITMAPCONTEXT* pBitmapContext);
extern "C" void unload_png_ios(LWBITMAPCONTEXT* pBitmapContext);
#endif

#if LW_PLATFORM_WIN32
static VOID GetImageFromFile(LPCWSTR file, IWICBitmap** bitmap)
{
	IWICImagingFactory* factory = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* converter = nullptr;

	//
	// Using 'CLSID_WICImagingFactory1' instead of 'CLSID_WICImagingFactory'
	// for runtime compatability on Windows 7.
	// https://stackoverflow.com/questions/16697112/why-using-wic-in-my-32-bit-application-fails-in-windows-7-32-bit
	//
	CoCreateInstance(CLSID_WICImagingFactory1, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
	factory->CreateDecoderFromFilename(file, nullptr, GENERIC_READ | GENERIC_WRITE, WICDecodeMetadataCacheOnDemand, &decoder);
	decoder->GetFrame(0, &frame);
	factory->CreateFormatConverter(&converter);
	converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
	factory->CreateBitmapFromSource(frame, WICBitmapNoCache, bitmap);

	factory->Release();
	decoder->Release();
	frame->Release();
	converter->Release();
}
#endif

extern "C" HRESULT init_ext_image_lib()
{
#if LW_PLATFORM_WIN32
	return CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#else
    return 0;
#endif
}

extern "C" char* load_software_decode_etc1_rgb(short extended_width, short extended_height, const char* d)
{
	// ETC1 Software decode: RGB
	char* rgb_data = (char*)malloc(extended_width * extended_height * 3);

	etc1_decode_image(
		(const etc1_byte*)d,
		(etc1_byte*)rgb_data,
		extended_width,
		extended_height,
		3,
		3 * extended_width);

	return rgb_data;
}

extern "C" void release_software_decode_etc1_rgb(char* d)
{
	free(d);
}

extern "C" void create_image(const char* filename, LWBITMAPCONTEXT* pBitmapContext, int tex_atlas_index)
{
#if LW_PLATFORM_WIN32
	IWICBitmap* bitmap = nullptr;
	IWICBitmapLock* lock = nullptr;
#endif

	size_t filename_wide_len = strlen(filename);
	if (strcmp(filename + filename_wide_len - 4, ".pkm") == 0)
	{
		size_t b_size;
		char* b = create_binary_from_file(filename, &b_size);

		LWPKM* pPkm = (LWPKM*)b;

		const int extended_width = swap_bytes(pPkm->extended_width);
		const int extended_height = swap_bytes(pPkm->extended_height);

		// ETC1 Software decode: RGB
		char* rgb_data = (char*)malloc(extended_width * extended_height * 3);

		etc1_decode_image(
			(const etc1_byte*)b + sizeof(LWPKM),
			(etc1_byte*)rgb_data,
			extended_width,
			extended_height,
			3,
			3 * extended_width);

		release_binary(b);

		char* rgba_data = (char*)malloc(extended_width * extended_height * 4);

		// change data format from BGRA to RGBA (since OpenGL ES does not support BGRA format)
		for (int i = 0; i < extended_width * extended_height; i++)
		{
			rgba_data[4 * i + 0] = rgb_data[3 * i + 0];
			rgba_data[4 * i + 1] = rgb_data[3 * i + 1];
			rgba_data[4 * i + 2] = rgb_data[3 * i + 2];
			rgba_data[4 * i + 3] = (char)0xff;
		}

		free(rgb_data);

		pBitmapContext->bitmap = 0;
		pBitmapContext->lock = 0;
		pBitmapContext->data = rgba_data;
		pBitmapContext->width = extended_width;
		pBitmapContext->height = extended_width;
	}
	else
	{
#if LW_PLATFORM_WIN32
		wchar_t filename_wide[1024];
		mbstowcs(filename_wide, filename, 1024);

		GetImageFromFile(filename_wide, &bitmap);

		WICRect r = { 0 };
		bitmap->GetSize(reinterpret_cast<UINT*>(&r.Width), reinterpret_cast<UINT*>(&r.Height));

		BYTE* data = nullptr;
		UINT len = 0;

		bitmap->Lock(&r, WICBitmapLockRead, &lock);
		lock->GetDataPointer(&len, &data);

		// change data format from BGRA to RGBA (since OpenGL ES does not support BGRA format)
		for (int i = 0; i < r.Width * r.Height; i++)
		{
			BYTE pb = data[4 * i + 0];
			BYTE pr = data[4 * i + 2];

			data[4 * i + 0] = pr;
			data[4 * i + 2] = pb;
		}

		pBitmapContext->bitmap = bitmap;
		pBitmapContext->lock = lock;
		pBitmapContext->data = reinterpret_cast<char*>(data);
		pBitmapContext->width = r.Width;
		pBitmapContext->height = r.Height;
#elif LW_PLATFORM_OSX
        load_png_osx(filename, pBitmapContext);
#elif LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR
        load_png_ios(filename, pBitmapContext);
#else
        pBitmapContext->bitmap = 0;
        pBitmapContext->lock = 0;
        pBitmapContext->data = 0;
        pBitmapContext->width = 0;
        pBitmapContext->height = 0;
#endif
	}
}

extern "C" void release_image(LWBITMAPCONTEXT* pBitmapContext)
{
	if (pBitmapContext->lock)
	{
#if LW_PLATFORM_WIN32
		reinterpret_cast<IWICBitmapLock*>(pBitmapContext->lock)->Release();
		reinterpret_cast<IWICBitmap*>(pBitmapContext->bitmap)->Release();
#endif
	}
	else if (pBitmapContext->data)
	{
#if LW_PLATFORM_WIN32
		free(pBitmapContext->data);
#elif LW_PLATFORM_OSX
        unload_png_osx(pBitmapContext);
#elif LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR
        unload_png_ios(pBitmapContext);
#endif
	}
}
