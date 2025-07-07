#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <obs-module.h>
// dc：代表“Device Context”（设备上下文），这是 Windows GDI 中用于绘制和显示图形的关键概念。
struct dc_capture {
	gs_texture_t *texture;
	gs_texture_t *extra_texture;
	bool texture_written;// 在dc_capture_capture函数中调用dc_capture_get_dc函数返回hdc不为空，就将texture_written置为true;在dc_capture_render函数中判断valid && texture_written为true才进行渲染
	int x, y;
	uint32_t width;
	uint32_t height;

	bool compatibility;// 是否勾选多显示器的兼容性，影响dc_capture_init函数中是否调用CreateCompatibleDC。only availiable in BITBLT
	HDC hdc;// 一个兼容的设备上下文
	HBITMAP bmp, old_bmp;// bmp:使用 CreateDIBSection 创建一个 DIB（设备无关位图）;old_bmp:调用SelectObject函数将bmp选入设备上下文hdc中，返回之前的原有对象old_bmp
	BYTE *bits;// 指向CreateDIBSection函数中变量的指针，该变量接收指向 DIB 位值位置的指针。

	bool capture_cursor;// 是否勾选显示鼠标指针
	bool cursor_captured;// 在capture_cursor为true前提下，调用GetCursorInfo是否成功
	bool cursor_hidden;// 每隔CURSOR_CHECK_TIME 0.2fs检查一下当前活动窗口与捕获的窗口是不是同一个进程。如果是BITBLT捕获方式，更新capture.cursor_hidden字段，控制是否调用draw_cursor
	CURSORINFO ci;// 用于调用GetCursorInfo函数时存储光标的信息（例如位置、状态等）

	bool valid;// 表示texture是否创建成功
};

extern void dc_capture_init(struct dc_capture *capture, int x, int y, uint32_t width, uint32_t height, bool cursor,
			    bool compatibility);
extern void dc_capture_free(struct dc_capture *capture);

extern void dc_capture_capture(struct dc_capture *capture, HWND window);
extern void dc_capture_render(struct dc_capture *capture, bool texcoords_centered);
