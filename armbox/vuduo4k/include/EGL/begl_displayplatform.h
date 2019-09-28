/******************************************************************************
 *  Copyright (C) 2017 Broadcom. The term "Broadcom" refers to Broadcom Limited and/or its subsidiaries.
 ******************************************************************************/
#ifndef __BEGL_DISPLAYPLATFORM_H__
#define __BEGL_DISPLAYPLATFORM_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <EGL/begl_memplatform.h>

#include <stdint.h>
#include <stdlib.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

/*****************************************************************************
 *****************************************************************************
 *****                       DISPLAY INTERFACE                           *****
 *****************************************************************************
 *****************************************************************************/

#define BEGL_DEFAULT_PLATFORM 0

typedef enum
{
   BEGL_Increment = 0,
   BEGL_Decrement
} BEGL_RefCountMode;

typedef enum
{
   BEGL_WindowInfoWidth = 1,
   BEGL_WindowInfoHeight = 2,
   BEGL_WindowInfoFormat = 4,
   BEGL_WindowInfoSwapChainCount = 8,
   BEGL_WindowInfoBackBufferAge = 16,
} BEGL_WindowInfoFlags;

typedef struct
{
   uint32_t            width;                 /* Visible width of window in pixels */
   uint32_t            height;                /* Visible height of window in pixels */
   uint32_t            swapchain_count;       /* Number of buffers in the swap chain, or 0 to take defaults from egl */
   uint32_t            backBufferAge;         /* Age of last dequeued buffer */
} BEGL_WindowInfo;

typedef struct BEGL_PixmapInfo
{
   uint32_t            width;                 /* Visible width of pixmap in pixels */
   uint32_t            height;                /* Visible height of pixmap in pixels */
   BEGL_BufferFormat   format;
} BEGL_PixmapInfo;

typedef struct BEGL_PixmapInfoEXT
{
   uint32_t            magic;
   uint32_t            width;                 /* Visible width of pixmap in pixels */
   uint32_t            height;                /* Visible height of pixmap in pixels */
   uint32_t            miplevels;             /* Number of miplevels required, eTILED format only */
                                              /* must be 1 (default) for all other buffer formats */
   BEGL_BufferFormat   format;
   BEGL_Colorimetry    colorimetry;           /* RGB, 601, 709 etc. */
   bool                secure;                /* Create pixmap in secure heap */
} BEGL_PixmapInfoEXT;

typedef struct
{
   uint32_t            width;                 /* Visible width of surface in pixels                     */
   uint32_t            height;                /* Visible height of surface in pixels                    */
   uint32_t            pitchBytes;            /* Bytes per row, or HW specific pitch for eTILED format  */
   uint64_t            physicalOffset;        /* Physical address                                       */
   void                *cachedAddr;           /* Cached address mapping                                 */
   uint32_t            byteSize;              /* Size of buffer in bytes                                */
   uint32_t            miplevels;             /* Number of miplevels contained in the surface,          */
                                              /* this will be 1 for formats that do not support mipmaps */
   BEGL_BufferFormat   format;
   BEGL_Colorimetry    colorimetry;
   bool                secure;                /* In secure memory                                       */
   bool                contiguous;            /* In contiguous memory                                   */

   // Extra data required for sand striped formats
   uint64_t            chromaOffset;          /* Physical address of chroma buffer                      */
   uint32_t            chromaByteSize;        /* Size of the chroma buffer in bytes                     */
   uint32_t            stripeWidth;           /* 128 or 256                                             */
   uint32_t            lumaStripedHeight;
   uint32_t            chromaStripedHeight;
   bool                lumaAndChromaInSameAllocation;

} BEGL_SurfaceInfo;

typedef void  *BEGL_DisplayHandle;    /* Opaque 'display' handle */

typedef struct BEGL_InitInterface
{
   /* Context pointer - opaque to the 3d driver code, but passed out in all
    * function pointer calls. Prevents the client code needing to perform
    * context lookups.
    */
   void *context;

   /*
    * Called from eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS)
    *
    * Returns a pointer to a static string containing a list of client
    * extensions implemented by the platform, independent of any display.
    *
    * This function is optional, it may be NULL or it may return NULL.
    */
   const char *(*GetClientExtensions)(void *context);

   /* Called from eglGetDisplay().
    *
    * Returns EGL_SUCCESS or an error defined by a platform extension.
    *
    * Sets opaque native display handle to a non-NULL value if a display
    * matching passed-in parameters could be found or created.
    * Multiple calls with the same parameters must always return the same
    * display handle.
    *
    * Note: the attribList may contain EGLint or EGLAttrib type attributes,
    *       which are different size on 64-bit platforms.
    */
   int32_t (*GetDisplay)(void *context, uint32_t platform,
         void *nativeDisplay, const void *attribList, bool isEglAttrib,
         BEGL_DisplayHandle *handle);

   /*
    * Called from eglInitialize().
    *
    * A successful initialisation must call BEGL_RegisterDisplayInterface()
    * with non-NULL display interface.
    */
   BEGL_Error (*Initialize)(void *context, BEGL_DisplayHandle handle);

   /*
    * Called from eglTerminate(), eglMakeCurrent() or eglReleaseTherad().
    *
    * A successful termination must call BEGL_RegisterDisplayInterface()
    * with NULL display interface.
    */
   BEGL_Error (*Terminate)(void *context, BEGL_DisplayHandle handle);
} BEGL_InitInterface;

typedef struct BEGL_DisplayInterface
{
   /* Context pointer - opaque to the 3d driver code, but passed out in all function pointer calls.
    * Prevents the client code needing to perform context lookups. */
   void *context;

   /* Called to determine current size of the window referenced by the opaque window handle.
    * Also fills in the number of pre-allocated swap-chain buffers, which must be > 0.
    * Set to the number of buffers in your pre-allocated chain. See BufferGet().
    * This is needed by EGL in order to know the size of a native 'window'. */
   BEGL_Error (*WindowGetInfo)(void *context, void *opaqueNativeWindow, BEGL_WindowInfoFlags flags, BEGL_WindowInfo *info);

   /* Called to get access to an underlying native surface.
    * Can be NULL if creating EGL images from native buffers isn't required.
    */
   BEGL_Error (*GetNativeSurface)(void *context, uint32_t eglTarget, void *eglClientBuffer, void **opaqueNativeSurface);

   BEGL_Error (*SurfaceGetInfo)(void *context, void *opaqueNativeSurface, BEGL_SurfaceInfo *info);
   BEGL_Error (*SurfaceChangeRefCount)(void *context, void *opaqueNativeSurface, BEGL_RefCountMode inOrDec);

   /* Return the next render buffer surface in the swap chain (in opaqueNativeSurface)
    * with a fence to wait on before accessing the buffer surface.
    * A surface obtained this way must be returned to the display system with a call to
    * DisplaySurface or CancelSurface.
    * All these 3 functions must be implemented;
    */
   BEGL_Error (*GetNextSurface)(void *context, void *opaqueNativeWindow, BEGL_BufferFormat format,
                               BEGL_BufferFormat *actualFormat, void **opaqueNativeSurface, bool secure, int *fence);

   BEGL_Error (*DisplaySurface)(void *context, void *nativeWindow, void *nativeSurface, int fence, int interval);

   BEGL_Error (*CancelSurface)(void *context, void *nativeWindow, void *nativeSurface, int fence);

   void *(*WindowPlatformStateCreate)(void *context, void *nativeWindow);

   BEGL_Error (*WindowPlatformStateDestroy)(void *context, void *windowState);

   BEGL_Error (*GetNativeFormat)(void *context, BEGL_BufferFormat format, uint32_t *nativeformat);

   /*
    * Called from eglQueryString(dpy, EGL_EXTENSIONS)
    *
    * Returns a pointer to a static string containing a list of display
    * extensions implemented by the platform for this display.
    *
    * This function is optional, it may be NULL or it may return NULL.
    */
   const char *(*GetDisplayExtensions)(void *context);

   bool (*BindWaylandDisplay)(void *context, void *egl_display, void *wl_display);

   bool (*UnbindWaylandDisplay)(void *context, void *egl_display, void *wl_display);

   bool (*QueryBuffer)(void *context, void *display, void* buffer, int32_t attribute, int32_t *value);

} BEGL_DisplayInterface;

extern void BEGL_RegisterInitInterface(BEGL_InitInterface *iface);
extern void BEGL_RegisterDisplayInterface(BEGL_DisplayInterface *iface);
extern void BEGL_PlatformAboutToShutdown(void);

typedef void (*PFN_BEGL_RegisterDisplayInterface)(BEGL_DisplayInterface *);
typedef void (*PFN_BEGL_PlatformAboutToShutdown)(void);

#ifdef __cplusplus
}
#endif

#endif /* __BEGL_DISPLAYPLATFORM_H__ */
