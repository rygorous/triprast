#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "types.h"
#include "vertex.h"

static HWND hwnd = 0;

void panic(const char *fmt, ...)
{
	char buffer[2048];
	va_list arg;
	va_start(arg, fmt);
	vsprintf_s(buffer, fmt, arg);
	va_end(arg);

	MessageBoxA(hwnd, buffer, "Error", MB_ICONERROR|MB_OK);
	exit(1);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_ERASEBKGND:
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_CHAR:
		if (wparam == 27) { // escape
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		break;
	}

	return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static HWND create_window(HINSTANCE hInst, const char *title, int width, int height)
{
	WNDCLASSA wc = {};

	wc.lpfnWndProc = wndproc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = "ryg.2d";
	if (!RegisterClassA(&wc))
		panic("RegisterClass failed!");

	DWORD style = WS_OVERLAPPEDWINDOW;
	RECT rc = { 0, 0, width, height };
	AdjustWindowRect(&rc, style, FALSE);
	HWND hwnd = CreateWindowA("ryg.2d", title, style, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);
	if (!hwnd)
		panic("CreateWindow failed!");

	return hwnd;
}

static bool msg_loop()
{
	bool ok = true;
	MSG msg;

	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT)
			ok = false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return ok;
}

static void display_img(HWND hwnd, const Pixel *data, int w, int h)
{
	RECT rc;
	if (!GetClientRect(hwnd, &rc))
		return;

	HDC hdc = GetDC(hwnd);
	BITMAPINFOHEADER bih = {};
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = w;
	bih.biHeight = -h;
	bih.biPlanes = 1;
	bih.biBitCount = 32;
	bih.biCompression = BI_RGB;
	SetDIBitsToDevice(hdc, 0, 0, w, h, 0, 0, 0, h, data, (BITMAPINFO *) &bih, DIB_RGB_COLORS);

	if (w < rc.right) {
		RECT fill = rc;
		fill.left = w;
		fill.bottom = h;
		FillRect(hdc, &fill, (HBRUSH) (COLOR_WINDOW + 1));
	}

	if (h < rc.bottom) {
		RECT fill = rc;
		fill.top = h;
		FillRect(hdc, &fill, (HBRUSH) (COLOR_WINDOW + 1));
	}

	ReleaseDC(hwnd, hdc);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
	static const int width = 800;
	static const int height = 600;

	hwnd = create_window(hInst, "trirast", width, height);
	ShowWindow(hwnd, nCmdShow);

	Pixel *framebuf = new Pixel[width * height];
	memset(framebuf, 0, width * height * sizeof(Pixel));

    // right-handed coord system: +x = right, +y = down, +z = into
    static const Vertex cube_verts[] = {
        //  x      y      z     u     v
        // front face
        { -1.0f, -1.0f, -1.0f, 0.0f, 0.0f },
        {  1.0f, -1.0f, -1.0f, 1.0f, 0.0f },
        {  1.0f,  1.0f, -1.0f, 1.0f, 1.0f },
        { -1.0f,  1.0f, -1.0f, 0.0f, 1.0f },
        // right face
        {  1.0f, -1.0f, -1.0f, 0.0f, 0.0f },
        {  1.0f, -1.0f,  1.0f, 1.0f, 0.0f },
        {  1.0f,  1.0f,  1.0f, 1.0f, 1.0f },
        {  1.0f,  1.0f, -1.0f, 0.0f, 1.0f },
        // back face
        {  1.0f, -1.0f,  1.0f, 0.0f, 0.0f },
        { -1.0f, -1.0f,  1.0f, 1.0f, 0.0f },
        { -1.0f,  1.0f,  1.0f, 1.0f, 1.0f },
        {  1.0f,  1.0f,  1.0f, 0.0f, 1.0f },
        // left face
        { -1.0f, -1.0f,  1.0f, 0.0f, 0.0f },
        { -1.0f, -1.0f, -1.0f, 1.0f, 0.0f },
        { -1.0f,  1.0f, -1.0f, 1.0f, 1.0f },
        { -1.0f,  1.0f,  1.0f, 0.0f, 1.0f },
        // top face
        { -1.0f, -1.0f,  1.0f, 0.0f, 0.0f },
        {  1.0f, -1.0f,  1.0f, 1.0f, 0.0f },
        {  1.0f, -1.0f, -1.0f, 1.0f, 1.0f },
        { -1.0f, -1.0f, -1.0f, 0.0f, 1.0f },
        // bottom face
        { -1.0f,  1.0f, -1.0f, 0.0f, 0.0f },
        {  1.0f,  1.0f, -1.0f, 1.0f, 0.0f },
        {  1.0f,  1.0f,  1.0f, 1.0f, 1.0f },
        { -1.0f,  1.0f,  1.0f, 0.0f, 1.0f },
    };
    static const U32 cube_inds[(3*2) * 6] = {
         0, 1, 2,  0, 2, 3, // front
         4, 5, 6,  4, 6, 7, // right
         8, 9,10,  8,10,11, // back
        12,13,14, 12,14,15, // left
        16,17,18, 16,18,19, // top
        20,21,22, 20,22,23, // bottom
    };

	while (msg_loop()) {
		display_img(hwnd, framebuf, width, height);
	}

	return 0;
}