#include <windows.h>
#include <vector>
#include <string>
#include <list>
#include <assert.h>

#include <avakar/di.h>
using avakar::di;

#include "css.h"
#include "dom.h"

float resolve_dim(css_dimension const & dim, float parent, float autov = -1.f)
{
	if (std::get_if<css_auto>(&dim))
		return autov;

	if (auto * perc = std::get_if<css_percentage>(&dim))
		return parent * perc->value;

	auto & len = std::get<css_length>(dim);
	return len.value;
}


struct fmt_context
{
	struct line_box
	{
		float x, y, dx, dy;

		dom::node * first_;
		size_t first_idx_;

		dom::node * last_;
		size_t last_idx_;
	};

	float x, dx;

	float margin_top, margin_bottom;

	std::vector<line_box> line_boxes;
	bool line_open = false;

	void open_line(dom::node * n, size_t idx)
	{
		if (!line_open)
		{
			line_boxes.emplace_back();
			line_open = true;

			auto & line = line_boxes.back();
			line.first_ = n;
			line.first_idx_ = idx;
		}
	}
};

#include <memory>
#include <string_view>

struct font
{
	virtual void measure(std::string_view text) = 0;
};

struct font_factory
{
	virtual std::shared_ptr<font> create_font() = 0;
};


struct layouter
{
	using deps_t = di<font_factory *>;

	layouter(deps_t deps)
		: deps_(deps)
	{
	}

	void layout_block(dom::element * elem, fmt_context & fmt, float x, float y, float dx)
	{
		auto font = deps_.get<font_factory *>()->create_font();

		for (dom::node * n = elem->first_child_; n != nullptr; n = n->next_)
		{
			if (n->type_ == dom::node_type::text)
			{
				dom::text_node * t = static_cast<dom::text_node *>(n);
				font->measure(t->value);
			}
		}
	}

private:
	deps_t deps_;
};

void layout_block(dom::element * elem, fmt_context & fmt, float x, float y, float dx)
{
/*	float w = resolve_dim(root->width, containing_block.dx);
	float margin_left = resolve_dim(root->margin_left, containing_block.dx);
	float margin_right = resolve_dim(root->margin_right, containing_block.dx);

	if (w == -1.f)
	{
		if (margin_left == -1.f)
			margin_left = 0;
		if (margin_right == -1.f)
			margin_right = 0;

		w = containing_block.dx - margin_left - margin_right;
	}
	else if (margin_left == -1.f && margin_right == -1.f)
	{
		margin_left = margin_right = (containing_block.dx - w) / 2;
	}
	else if (margin_left == -1.f)
	{
		margin_left = containing_block.dx - margin_right - w;
	}
	else
	{
		margin_right = containing_block.dx - margin_left - w;
	}*/


}

extern "C" ULONG __ImageBase;

struct gdi_text_measurer
{
	virtual void measure(HFONT font, std::string_view text) = 0;
};

struct gdi_font final
	: font
{
	gdi_font(gdi_text_measurer & measurer, HFONT hfont)
		: measurer_(measurer), hfont_(hfont)
	{
	}

	~gdi_font()
	{
		DeleteObject(hfont_);
	}

	void measure(std::string_view text) override
	{
	}

private:
	gdi_text_measurer & measurer_;
	HFONT hfont_;
};

struct window final
	: gdi_text_measurer, font_factory
{
	window()
	{
		WNDCLASSEXW wce = {};
		wce.cbSize = sizeof wce;
		wce.hInstance = (HINSTANCE)&__ImageBase;
		wce.lpszClassName = L"MainWindowClass";
		wce.lpfnWndProc = &wndproc;
		ATOM cls = RegisterClassExW(&wce);

		hwnd_ = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, (LPCWSTR)(WORD)cls, L"Main window", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr,
			nullptr, (HINSTANCE)&__ImageBase, this);

		ShowWindow(hwnd_, SW_SHOW);
	}

	std::shared_ptr<font> create_font() override
	{
		HFONT hfont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");

		return std::make_shared<gdi_font>(*this, hfont);
	}

private:
	HWND hwnd_;

	HDC backdc_ = nullptr;
	HBITMAP backbmp_ = nullptr;
	int width_;
	int height_;

	HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));

	void measure(HFONT font, std::string_view text) override
	{
	}

	void on_paint()
	{
		HDC dc = GetDC(hwnd_);
		BitBlt(dc, 0, 0, width_, height_, backdc_, 0, 0, SRCCOPY);
		ReleaseDC(hwnd_, dc);
		ValidateRect(hwnd_, nullptr);
	}

	void on_resize(int width, int height)
	{
		width_ = width;
		height_ = height;

		if (backbmp_)
			DeleteObject(backbmp_);
		if (backdc_)
			DeleteDC(backdc_);

		HDC dc = GetDC(hwnd_);

		backdc_ = CreateCompatibleDC(dc);
		backbmp_ = CreateCompatibleBitmap(dc, width, height);


		SelectObject(backdc_, backbmp_);

		RECT rect = { 100, 100, 200, 200 };
		FillRect(backdc_, &rect, brush);

		ReleaseDC(hwnd_, dc);

		InvalidateRect(hwnd_, nullptr, FALSE);
	}

	static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		window * self;

		if (msg == WM_CREATE)
		{
			self = (window *)((CREATESTRUCTW *)lparam)->lpCreateParams;
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG)self);
		}
		else
		{
			self = (window *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		}

		switch (msg)
		{
		case WM_PAINT:
			self->on_paint();
			return 0;
		case WM_SIZE:
			self->on_resize(LOWORD(lparam), HIWORD(lparam));
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}

};

int main(int argc, char * argv[])
{
	dom::document doc;
	dom::element * root = doc.create_root();

	window wnd;

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return msg.wParam;
}
