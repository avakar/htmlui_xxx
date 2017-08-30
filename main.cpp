#include <windows.h>
#include <vector>
#include <string>
#include <list>
#include <assert.h>
#include <algorithm>
#include <map>

#include <avakar/di.h>
using avakar::di;

#include "css.h"
#include "dom.h"

/*int resolve_dim(css_dimension const & dim, int parent, int autov = -1.f)
{
	if (std::get_if<css_auto>(&dim))
		return autov;

	if (auto * perc = std::get_if<css_percentage>(&dim))
		return parent * perc->value;

	auto & len = std::get<css_length>(dim);
	return len.value;
}*/


struct fmt_context
{
	struct line_box
	{
		int y, dy;
		int consumed_dx = 0;

		int max_asc = 0;
		int max_desc = 0;

		dom::text * first_;
		size_t first_idx_;

		dom::text * last_;
		size_t last_idx_;
	};

	int x, dx;

	int margin_top, margin_bottom;

	std::vector<line_box> line_boxes;
	bool line_open = false;

	size_t current_line()
	{
		return line_open? line_boxes.size() - 1: line_boxes.size();
	}

	void append_chunk(dom::text * n, size_t first, size_t last, int w, int asc, int desc)
	{
		if (!line_open)
		{
			int y = line_boxes.empty() ? 0 : line_boxes.back().y + line_boxes.back().dy;

			line_boxes.emplace_back();
			line_open = true;

			auto & lb = line_boxes.back();
			lb.y = y;
			lb.first_ = n;
			lb.first_idx_ = first;
		}

		auto & lb = line_boxes.back();
		lb.last_ = n;
		lb.last_idx_ = last;

		lb.consumed_dx += w;

		lb.max_asc = (std::max)(lb.max_asc, asc);
		lb.max_desc = (std::max)(lb.max_desc, desc);
	}

	void line_break()
	{
		if (!line_open)
			return;

		auto & lb = line_boxes.back();
		lb.dy = lb.max_asc + lb.max_desc;
		line_open = false;
	}

	int remaining()
	{
		return dx - this->next_x();
	}

	int next_x()
	{
		return !line_open? 0: line_boxes.back().consumed_dx;
	}
};

#include <memory>
#include <string_view>

struct font
{
	HFONT hfont;
};

struct brush
{
	HBRUSH hbrush;
};

void layout_block(dom::element * elem, fmt_context & fmt, int x, int y, int dx)
{
/*	int w = resolve_dim(root->width, containing_block.dx);
	int margin_left = resolve_dim(root->margin_left, containing_block.dx);
	int margin_right = resolve_dim(root->margin_right, containing_block.dx);

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

struct font_id
{
	std::string name;
	int size;
	bool italic;

	bool operator<(font_id const & rhs) const
	{
		return std::tie(name, size, italic) < std::tie(rhs.name, rhs.size, rhs.italic);
	}
};

struct window final
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

	void set_document(dom::element * doc)
	{
		document_ = doc;
		this->relayout();
	}

	dom::element * document_ = nullptr;

private:
	HWND hwnd_;

	HDC backdc_ = nullptr;
	HBITMAP backbmp_ = nullptr;
	int width_;
	int height_;
	std::map<font_id, std::weak_ptr<font>> font_cache_;
	std::map<css::color, std::weak_ptr<brush>> brush_cache_;

	static std::wstring to_utf16(std::string_view str)
	{
		std::wstring res;
		int len = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), nullptr, 0);
		res.resize(len + 1);
		MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), &res[0], res.size());
		res.pop_back();
		return res;
	}

	std::shared_ptr<font> get_font(css::style const & style)
	{
		int font_size = std::get<css::length>(style.font_size).value;

		font_id id{ style.font_family, font_size, style.font_style == css::font_style::kw_italic };
		auto it = font_cache_.find(id);
		if (it != font_cache_.end())
		{
			if (auto r = it->second.lock())
				return r;
		}

		auto r = std::make_shared<font>();
		r->hfont = CreateFontW(-id.size, 0, 0, 0, FW_NORMAL, id.italic, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, to_utf16(id.name).c_str());
		font_cache_[id] = r;
		return r;
	}

	std::shared_ptr<brush> get_brush(css::color const & color)
	{
		auto it = brush_cache_.find(color);
		if (it != brush_cache_.end())
		{
			if (auto r = it->second.lock())
				return r;
		}

		auto r = std::make_shared<brush>();
		r->hbrush = CreateSolidBrush(RGB(color.r, color.g, color.b));
		brush_cache_[color] = r;
		return r;
	}

	void layout_block(dom::element * elem, fmt_context & fmt)
	{
		elem->font = this->get_font(*elem->style);
		elem->brush = this->get_brush(elem->style->background_color);

		for (dom::node * n = elem->first_child_; n != nullptr; n = n->next_)
		{
			if (n->type_ == dom::node_type::element)
			{
				dom::element * e = static_cast<dom::element *>(n);
				layout_block(e, fmt);
			}
			else if (n->type_ == dom::node_type::text)
			{
				dom::text * t = static_cast<dom::text *>(n);
				if (t->value.empty())
					continue;

				t->render_value = to_utf16(t->value);
				SelectObject(backdc_, elem->font->hfont);

				TEXTMETRICW tm;
				GetTextMetricsW(backdc_, &tm);

				std::vector<int> partial_widths(t->render_value.size());

				t->x = fmt.next_x();
				t->y = fmt.current_line();

				size_t first = 0;
				for (;;)
				{
					int fit;
					SIZE size;
					GetTextExtentExPointW(backdc_, t->render_value.data() + first, t->render_value.size() - first, fmt.remaining(), &fit, partial_widths.data(), &size);

					size_t last;
					int width;

					if (fit == t->render_value.size() - first)
					{
						last = t->render_value.size();
						width = size.cx;
					}
					else
					{
						for (last = fit + first; last > first; --last)
						{
							if (t->render_value[last - 1] == ' ')
							{
								break;
							}
						}

						if (last == first)
						{
							for (last = fit + first; last < t->render_value.size(); ++last)
							{
								if (t->render_value[last] == ' ')
									break;
							}

							GetTextExtentPoint32W(backdc_, t->render_value.data() + first, last - first, &size);
							width = size.cx;
						}
						else
						{
							--last;
							width = partial_widths[last - first - 1];
						}
					}

					fmt.append_chunk(t, first, last, width, tm.tmAscent, tm.tmDescent);
					if (last != t->render_value.size())
						fmt.line_break();

					if (last == t->render_value.size())
						break;

					first = last + 1;
				}

				t->dy = fmt.current_line();
			}
		}
	}

	void render_block(dom::element * elem, fmt_context * ctx)
	{
		if (elem->fmt_context)
			ctx = elem->fmt_context.get();

		for (dom::node * n = elem->first_child_; n != nullptr; n = n->next_)
		{
			if (n->type_ == dom::node_type::element)
			{
				dom::element * e = static_cast<dom::element *>(n);
				render_block(e, ctx);
			}
			else if (n->type_ == dom::node_type::text)
			{
				dom::text * t = static_cast<dom::text *>(n);

				SelectObject(backdc_, elem->font->hfont);

				for (int line = t->y; line <= t->dy; ++line)
				{
					auto & lb = ctx->line_boxes[line];

					size_t first = 0;
					size_t last = t->render_value.size();
					int x = t->x;
					if (lb.first_ == t)
					{
						first = lb.first_idx_;
						x = 0;
					}
					if (lb.last_ == t)
						last = lb.last_idx_;

					TextOutW(backdc_, x, lb.y, t->render_value.c_str() + first, last - first);
				}
			}
		}
	}

	void on_paint()
	{
		HDC dc = GetDC(hwnd_);
		BitBlt(dc, 0, 0, width_, height_, backdc_, 0, 0, SRCCOPY);
		ReleaseDC(hwnd_, dc);
		ValidateRect(hwnd_, nullptr);
	}

	void relayout()
	{
		if (!document_)
			return;

		fmt_context fmt_ctx;
		fmt_ctx.x = 0;
		fmt_ctx.dx = width_;
		this->layout_block(document_, fmt_ctx);

		RECT rect = { 0, 0, width_, height_ };
		FillRect(backdc_, &rect, document_->brush->hbrush);

		this->render_block(document_, &fmt_ctx);
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
		ReleaseDC(hwnd_, dc);

		SelectObject(backdc_, backbmp_);
		SetBkMode(backdc_, TRANSPARENT);

		this->relayout();
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
	css::style body_style;
	body_style.font_family = "Tahoma";
	body_style.font_size = css::length{ 12};
	body_style.background_color = css::color{255, 255, 255};

	css::style em_style;
	em_style.font_family = "Tahoma";
	em_style.font_size = css::length{ 20 };
	em_style.font_style = css::font_style::kw_italic;

	dom::element root;
	root.style = std::shared_ptr<css::style>(std::shared_ptr<void>(), &body_style);

	root.append_text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque eu est velit."
		"Ut dictum massa et mauris mattis suscipit. Proin finibus ultrices convallis. Curabitur tempus semper mauris, vel tempus dolor efficitur in. ");

	auto * em = root.append_element("em");
	em->style = std::shared_ptr<css::style>(std::shared_ptr<void>(), &em_style);

	em->append_text(
		"Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Suspendisse potenti.Phasellus sed auctor mi. ");

	root.append_text(
		"Mauris nec volutpat libero, sit amet maximus augue. Proin ac maximus ante, eu convallis elit. Donec rutrum et arcu ac cursus.");

	window wnd;
	wnd.set_document(&root);

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	return msg.wParam;
}
