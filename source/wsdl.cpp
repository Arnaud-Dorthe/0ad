/*
 * emulation of a subset of SDL and GLUT for Win32
 *
 * Copyright (c) 2002 Jan Wassenberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Contact info:
 *   Jan.Wassenberg@stud.uni-karlsruhe.de
 *   http://www.stud.uni-karlsruhe.de/~urkt/
 */

#include <stdio.h>
#include <stdlib.h>

#include <process.h>

#include "wsdl.h"
#include "win.h"
#include "misc.h"


/* state */
static bool app_active;		/* is window active & on top?
							   if not, msg loop should yield */
static bool fullscreen;		/* in fullscreen mode?
							   if so, restore mode when app is deactivated */

HWND hWnd = 0;				/* available to the app for ShowWindow calls, etc. */

static DEVMODE dm;			/* current video mode */
static HDC hDC;
static HGLRC hGLRC;

static int z_depth = 24;	/* depth buffer size; set via SDL_GL_SetAttribute */

static u16 mouse_x, mouse_y;



/*
 * shared msg handler
 * SDL and GLUT have separate pumps; messages are handled there
 */
static LRESULT CALLBACK wndproc(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 0;

	case WM_ERASEBKGND:
		return 0;

	// prevent screensaver / monitor power off
	case WM_SYSCOMMAND:
		if(wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)
			return 0;
		break;

	case WM_ACTIVATE:
		app_active = (wParam & 0xffff) != 0;

		if(fullscreen)
		{
			if(app_active)
				ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
			else
				ChangeDisplaySettings(0, 0);
		}
		break;

	case WM_CLOSE:
		exit(0);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

  
int SDL_PollEvent(SDL_Event* ev)	/* ev must be valid */
{
	/* windows messages */
	MSG msg;
	while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		/* key */
		if(msg.message == WM_KEYUP || msg.message == WM_KEYDOWN)
		{
			ev->type = (u8)((msg.message == WM_KEYUP)? SDL_KEYUP : SDL_KEYDOWN);
			ev->key.keysym.sym = (SDLKey)msg.wParam;
			return 1;
		}

		if(msg.message == WM_SYSCOMMAND)
			ev->type = 0;

		/* mouse click */
		uint button;
		for(button = 0; button < 3; button++)
		{
			if(msg.message == button+WM_LBUTTONDOWN)
				ev->type = SDL_MOUSEBUTTONDOWN;
			else if(msg.message == button+WM_LBUTTONUP)
				ev->type = SDL_MOUSEBUTTONUP;
		}
		if(button < 3)
		{
			ev->button.button = (u8)button;
			ev->button.x = (u16)(msg.lParam & 0xffff);
			ev->button.y = (u16)((msg.lParam >> 16) & 0xffff);
			return 1;
		}

		/* active */
		if(msg.message == WM_ACTIVATE)
		{
			ev->type = SDL_ACTIVE;
			ev->active.gain = app_active;
			ev->active.state = 0;
			return 1;
		}
	}

	/*
	 * mouse motion
	 *
	 * don't use DirectInput, because we want to respect the user's mouse sensitivity
	 * Windows messages are laggy, so poll instead.
	 */
	POINT p;
	GetCursorPos(&p);
	if(mouse_x != p.x || mouse_y != p.y)
	{
		ev->type = SDL_MOUSEMOTION;
		ev->motion.x = mouse_x = (u16)p.x;
		ev->motion.y = mouse_y = (u16)p.y;
		return 1;
	}

	return 0;
}



int SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
	if(attr == SDL_GL_DEPTH_SIZE)
		z_depth = value;

	return 0;
}


/*
 * set video mode wxh:bpp if necessary.
 * w = h = bpp = 0 => no change.
 */
int SDL_SetVideoMode(int w, int h, int bpp, u32 flags)
{
	fullscreen = (flags & SDL_FULLSCREEN);

	/* get current mode settings */
	dm.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);
	int cur_w = dm.dmPelsWidth, cur_h = dm.dmPelsHeight;
	int cur_bpp = dm.dmBitsPerPel;

	/*
	 * check if mode needs to be changed
	 * (could split this out, but depends on fullscreen and dm)
	 */
	if(w != 0 && h != 0 && bpp != 0)
		if(/* wrong bit depth */
		   (bpp != cur_bpp) ||				
		   /* higher res mode needed */
		   (w > cur_w || h > cur_h) ||
		   /* fullscreen, and not exact mode */
		   (fullscreen && (w != cur_w || h != cur_h)))
	{
		dm.dmPelsWidth  = w;
		dm.dmPelsHeight = h;
		dm.dmBitsPerPel = bpp;	
		dm.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL;
	}
	// mode set at first WM_ACTIVATE

	/*
	 * window init
	 * create new window every time (instead of once at startup), 'cause
	 * pixel format isn't supposed to be changed more than once
	 */

	HINSTANCE hInst = GetModuleHandle(0);

	/* register window class */
	static WNDCLASS wc;
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = wndproc;
	wc.lpszClassName = "ogl";
	wc.hInstance = hInst;
	RegisterClass(&wc);

	hWnd = CreateWindowEx(0, "ogl", APP_NAME, WS_POPUP|WS_VISIBLE, 0, 0, dm.dmPelsWidth, dm.dmPelsHeight, 0, 0, hInst, 0);
	if(!hWnd)
		return 0;

	hDC = GetDC(hWnd);

	/* set pixel format */
	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		(BYTE)bpp,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		(BYTE)z_depth,
		0, 0,
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	int pf = ChoosePixelFormat(hDC, &pfd);
	if(!SetPixelFormat(hDC, pf, &pfd))
		return 0;

	hGLRC = wglCreateContext(hDC);
	if(!hGLRC)
		return 0;

	if(!wglMakeCurrent(hDC, hGLRC))
		return 0;

	return 1;
}


inline void SDL_GL_SwapBuffers()
{
	SwapBuffers(hDC);
}



static SDL_VideoInfo video_info;


#ifdef DDRAW7
#include <ddraw.h>
#ifdef _MSC_VER
#pragma comment(lib, "ddraw.lib")
#pragma comment(lib, "dxguid.lib")
#endif
#endif


int SDL_Init(u32 flags)
{
	UNUSED(flags)

#ifdef DDRAW7
	LPDIRECTDRAW7 dd;
	DirectDrawCreateEx(0, (void**)&dd, IID_IDirectDraw7, 0);
	static DDSCAPS2 caps;
	caps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
	dd->GetAvailableVidMem(&caps, (DWORD*)&video_info.video_mem, 0); 
	dd->Release();
#endif

	dm.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);

	return 0;
}


void SDL_Quit()
{
	DestroyWindow(hWnd);

	wglMakeCurrent(0, 0);
	wglDeleteContext(hGLRC);
	ChangeDisplaySettings(0, 0);
}


SDL_VideoInfo* SDL_GetVideoInfo()
{
	return &video_info;
}


SDL_Surface* SDL_GetVideoSurface()
{
	static SDL_Surface surf;
	surf.w = dm.dmPelsWidth;
	surf.h = dm.dmPelsHeight;
	return &surf;
}

__declspec(naked) u32 SDL_GetTicks()
{
__asm jmp	dword ptr [GetTickCount]
}


void* SDL_GL_GetProcAddress(const char* name)
{
	return wglGetProcAddress(name);
}


SDL_sem* SDL_CreateSemaphore(int cnt)
{
	return (SDL_sem*)CreateSemaphore(0, cnt, 0x7fffffff, 0);
}

void __stdcall SDL_DestroySemaphore(SDL_sem*)
{
__asm jmp	dword ptr [CloseHandle]
}

int SDL_SemPost(SDL_sem* sem)
{
	return ReleaseSemaphore(sem, 1, 0);
}

int SDL_SemWait(SDL_sem* sem)
{
	return WaitForSingleObject(sem, INFINITE);
}

SDL_Thread* SDL_CreateThread(int(*func)(void*), void* param)
{
	return (SDL_Thread*)_beginthread((void(*)(void*))func, 0, param);
}


int SDL_KillThread(SDL_Thread* thread)
{
	return TerminateThread(thread, 0);
}


__declspec(naked) int __stdcall SDL_WarpMouse(int, int)
{
__asm jmp	dword ptr [SetCursorPos]
}






















static bool need_redisplay;	/* display callback should be called in next main loop iteration */



/* glut callbacks */
static void (*idle)();
static void (*display)();
static void (*key)(int, int, int);
static void (*special)(int, int, int);
static void (*mouse)(int, int, int, int);

void glutIdleFunc(void (*func)())
{ idle = func; }

void glutDisplayFunc(void (*func)())
{ display = func; }

void glutKeyboardFunc(void (*func)(int, int, int))
{ key = func; }

void glutSpecialFunc(void (*func)(int, int, int))
{ special = func; }

void glutMouseFunc(void (*func)(int, int, int, int))
{ mouse = func; }









void glutInit(int* argc, char* argv[])
{
	UNUSED(argc)
	UNUSED(argv)

	SDL_Init(0);
	atexit(SDL_Quit);
}


int glutGet(int arg)
{
	if(arg == GLUT_ELAPSED_TIME)
		return GetTickCount();

	dm.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &dm);

	if(arg == GLUT_SCREEN_WIDTH)
		return dm.dmPelsWidth;
	if(arg == GLUT_SCREEN_HEIGHT)
		return dm.dmPelsHeight;

	return 0;
}


static int w, h, bpp, refresh;

int glutGameModeString(const char* str)
{
	/* default = "don't care", in case string doesn't specify all values */
	w = 0, h = 0, bpp = 0, refresh = 0;

	sscanf(str, "%dx%d:%d@%d", &w, &h, &bpp, &refresh);

	return 1;
}


/*
 */
int glutEnterGameMode()
{
	return SDL_SetVideoMode(w, h, bpp, SDL_OPENGL|SDL_FULLSCREEN);
}



inline void glutPostRedisplay()
{
	need_redisplay = true;
}


void glutSetCursor(int cursor)
{
	SetCursor(LoadCursor(NULL, (const char*)cursor));
}



/*
 * GLUT message handler
 * message also goes to the shared wndproc
 *
 * not done in wndproc to separate GLUT and SDL;
 * split out of glutMainLoop for clarity.
 */
static void glut_process_msg(MSG* msg)
{
	switch(msg->message)
	{
	case WM_PAINT:
		need_redisplay = true;
		break;

	case WM_CHAR:
		if(key)
			key((int)msg->wParam, mouse_x, mouse_y);
		break;

	case WM_KEYDOWN:
		if(special)
			special((int)msg->wParam, mouse_x, mouse_y);
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:	/* FIXME: only left/right clicks, assume GLUT_LEFT|RIGHT_BUTTON == 0, 1 */
		if(mouse)
			mouse(msg->message == WM_RBUTTONDOWN, GLUT_DOWN, (int)(msg->lParam & 0xffff), (int)(msg->lParam >> 16));
		break;

	case WM_MOUSEWHEEL:
		if(mouse)
			mouse(GLUT_MIDDLE_BUTTON, ((short)(msg->wParam >> 16) > 0)? GLUT_UP : GLUT_DOWN, 0, 0);
		break;
	}
}


void glutMainLoop()
{
	for(;;)
	{
		if(!app_active)
			WaitMessage();

		MSG msg;
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			glut_process_msg(&msg);

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if(idle)
			idle();

		if(need_redisplay)
		{
			need_redisplay = false;
			display();
		}
	}
}
