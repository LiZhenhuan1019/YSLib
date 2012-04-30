﻿/*
	Copyright (C) by Franksoft 2010 - 2012.

	This file is part of the YSLib project, and may only be used,
	modified, and distributed under the terms of the YSLib project
	license, LICENSE.TXT.  By continuing to use, modify, or distribute
	this file you indicate that you have read the license and
	understand and accept it fully.
*/

/*!	\file Shells.cpp
\ingroup YReader
\brief Shell 框架逻辑。
\version r6094;
\author FrankHB<frankhb1989@gmail.com>
\since 早于 build 132 。
\par 创建时间:
	2010-03-06 21:38:16 +0800;
\par 修改时间:
	2012-04-30 22:18 +0800;
\par 文本编码:
	UTF-8;
\par 模块名称:
	YReader::Shells;
*/


#include "Shells.h"
#include "ShlReader.h"

////////
//测试用声明：全局资源定义。
//extern char gstr[128];

using namespace ystdex;

YSL_BEGIN_NAMESPACE(YReader)

namespace
{
	ResourceMap GlobalResourceMap;
}

DeclResource(GR_BGs)

namespace
{

using namespace YReader;

Color
GenerateRandomColor()
{
//使用 std::time(0) 初始化随机数种子在 DeSmuMe 上无效。
//	std::srand(std::time(0));
	return Color(std::rand(), std::rand(), std::rand(), 1);
}

//测试函数。

//背景测试。
void
dfa(BitmapPtr buf, SDst x, SDst y)
{
	//hypY1
	buf[y * MainScreenWidth + x] = Color(
		~(x * y) >> 2,
		x | y | 128,
		240 - ((x & y) >> 1)
	);
}

void
dfap(BitmapPtr buf, SDst x, SDst y)
{
	//bza1BRGx
	buf[y * MainScreenWidth + x] = Color(
		(x << 4) / (y | 1),
		(x | y << 1) % (y + 2),
		(~y | x << 1) % 27 + 3
	);
}

void
dfac1(BitmapPtr buf, SDst x, SDst y)
{
	//fl1RBGx
	buf[y * MainScreenWidth + x] = Color(
		x + y * y,
		(x & y) ^ (x | y),
		x * x + y
	);
}

void
dfac1p(BitmapPtr buf, SDst x, SDst y)
{
	//rz3GRBx
	buf[y * MainScreenWidth + x] = Color(
		(x * y) | x,
		(x * y) | y,
		(x ^ y) * (x ^ y)
	);
}

void
dfac2(BitmapPtr buf, SDst x, SDst y)
{
	//v1BGRx1
	buf[y * MainScreenWidth + x] = Color(
		(x << 4) / ((y & 1) | 1),
		~x % 101 + y,
		(x + y) % (((y - 2) & 1) | 129) + (x << 2)
	);
}

void
dfac2p(BitmapPtr buf, SDst x, SDst y)
{
	//arz1x
	buf[y * MainScreenWidth + x] = Color(
		(x | y) % (y + 2),
		(~y | x) % 27 + 3,
		(x << 6) / (y | 1)
	);
}

template<typename _tTarget>
_tTarget&
FetchGlobalResource(ResourceIndex idx)
{
	if(GlobalResourceMap[idx].IsEmpty())
		GlobalResourceMap[idx] = MakeValueObjectByPtr(new _tTarget());
	return GlobalResourceMap[GR_BGs].GetObject<_tTarget>();
}

shared_ptr<Image>&
FetchGlobalImage(size_t idx)
{
	auto& spi(FetchGlobalResource<array<shared_ptr<Image>, 10>>(GR_BGs));

	YAssert(IsInInterval(idx, 10u), "Index is out of range.");
	return spi[idx];
}

} // unnamed namespace;

YSL_END_NAMESPACE(YReader)


YSL_BEGIN

void
ReleaseShells()
{
	using namespace YReader;

	for(size_t i(0); i != 10; ++i)
		FetchGlobalImage(i).reset();
	GlobalResourceMap.clear();
	ReleaseStored<ShlReader>();
	ReleaseStored<ShlExplorer>();
}

YSL_END


YSL_BEGIN_NAMESPACE(YReader)

using namespace Shells;
using namespace Drawing::ColorSpace;

shared_ptr<Image>&
FetchImage(size_t i)
{
	//色块覆盖测试用程序段。
	const PPDRAW p_bg[10] = {nullptr, dfa, dfap, dfac1, dfac1p, dfac2, dfac2p};

	if(!FetchGlobalImage(i) && p_bg[i])
	{
		auto& h(FetchGlobalImage(i));

		if(!h)
			h = make_shared<Image>(nullptr, MainScreenWidth, MainScreenHeight);
		ScrDraw(h->GetBufferPtr(), p_bg[i]);
	}
	return FetchGlobalImage(i);
}


FPSCounter::FPSCounter(u64 s)
	: last_tick(GetHighResolutionTicks()), now_tick(), refresh_count(1),
	MinimalInterval(s)
{}

u32
FPSCounter::Refresh()
{
	const u64 tmp_tick(GetHighResolutionTicks());

	if(YCL_UNLIKELY(last_tick + MinimalInterval < tmp_tick))
	{
		last_tick = now_tick;

		const u32 r(1000000000000ULL * refresh_count
			/ ((now_tick = tmp_tick) - last_tick));

		refresh_count = 1;
		return r;
	}
	else
		++refresh_count;
	return 0;
}


namespace
{

shared_ptr<TextList::ListType>
GenerateList(const String& str)
{
	auto p(new TextList::ListType());

	p->push_back(str);

	char cstr[40];

	std::sprintf(cstr, "%p;", p);
	p->push_back(cstr);
	return share_raw(p);
}

void
SwitchVisible(IWidget& wgt)
{
	SetVisibleOf(wgt, !IsVisible(wgt));
	Invalidate(wgt);
}


namespace EnrtySpace
{
	typedef enum
	{
		Empty,
		Now,
		Parent,
		Directory,
		Text,
		Hex
	} EntryType;
}

bool
ReaderPathFilter(const string& path)
{
	const auto ext(IO::GetExtensionOf(path).c_str());

	return !strcasecmp(ext, "txt")
		|| !strcasecmp(ext, "c")
		|| !strcasecmp(ext, "cpp")
		|| !strcasecmp(ext, "h")
		|| !strcasecmp(ext, "hpp")
		|| !strcasecmp(ext, "ini")
		|| !strcasecmp(ext, "xml");
}

EnrtySpace::EntryType
GetEntryType(const string& path)
{
	using namespace EnrtySpace;

	if(path.length() == 0)
		return Empty;
	if(path == IO::FS_Now)
		return Now;
	if(path == IO::FS_Parent)
		return Parent;
	if(*path.rbegin() == YCL_PATH_DELIMITER)
		return Directory;
	if(ReaderPathFilter(path))
		return Text;
	return Hex;
}
EnrtySpace::EntryType
GetEntryType(const IO::Path& path)
{
	return GetEntryType(path.GetNativeString());
}

bool
CheckReaderEnability(FileBox& fb, CheckBox& hex)
{
	if(fb.IsSelected())
	{
		using namespace EnrtySpace;

		switch(GetEntryType(fb.GetList()[fb.GetSelectedIndex()]))
		{
		case Text:
			return true;
		case Hex:
			return hex.IsTicked();
		default:
			;
		}
	}
	return false;
}

} // unnamed namespace;


ShlExplorer::ShlExplorer(const IO::Path& path)
	: ShlDS(),
	lblTitle(Rect(16, 20, 220, 22)), lblPath(Rect(12, 80, 240, 22)),
	fbMain(Rect(4, 6, 248, 128)),
	btnTest(Rect(115, 165, 65, 22)), btnOK(Rect(185, 165, 65, 22)),
	chkFPS(Rect(210, 144, 13, 13)), chkHex(Rect(232, 144, 13, 13)),
	pWndTest(make_unique<TFormTest>()), pWndExtra(make_unique<TFormExtra>()),
	lblA(Rect(16, 44, 200, 22)), lblB(Rect(5, 120, 72, 22)),
	mhMain(*GetDesktopDownHandle()), fpsCounter(500000000ULL)
{
	auto& dsk_up(GetDesktopUp());
	auto& dsk_dn(GetDesktopDown());

	AddWidgets(dsk_up, lblTitle, lblPath, lblA, lblB),
	AddWidgets(dsk_dn, fbMain, btnTest, btnOK, chkFPS, chkHex),
	AddWidgetsZ(dsk_dn, DefaultWindowZOrder, *pWndTest, *pWndExtra),
	//对 fbMain 启用缓存。
	fbMain.SetRenderer(make_unique<BufferedRenderer>(true)),
	lblB.SetTransparent(true),
	SetVisibleOf(*pWndTest, false),
	SetVisibleOf(*pWndExtra, false),
	yunseq(
		tie(dsk_up.Background, dsk_dn.Background, lblTitle.Text, lblPath.Text,
			btnTest.Text, btnOK.Text, lblA.Text, lblB.Text) = forward_as_tuple(
			ImageBrush(FetchImage(1)), ImageBrush(FetchImage(2)),
			u"YReader", path, u"测试(X)", u"确定(A)",
			u"文件列表：请选择一个文件。", u"程序测试"),
		fbMain.SetPath(path),
		Enable(btnTest, true),
		Enable(btnOK, false),
	// TODO: show current working directory properly;
	//	lblTitle.Transparent = true,
	//	lblPath.Transparent = true,
		dsk_dn.BoundControlPtr = std::bind(&ShlExplorer::GetBoundControlPtr,
			this, std::placeholders::_1),
		FetchEvent<KeyUp>(dsk_dn) += OnKey_Bound_TouchUpAndLeave,
		FetchEvent<KeyDown>(dsk_dn) += OnKey_Bound_EnterAndTouchDown,
		FetchEvent<KeyPress>(dsk_dn) += OnKey_Bound_Click,
		fbMain.GetViewChanged() += [this](UIEventArgs&&){
			lblPath.Text = fbMain.GetPath();
			Invalidate(lblPath);
		},
		fbMain.GetSelected() += [this](IndexEventArgs&&){
			Enable(btnOK, CheckReaderEnability(fbMain, chkHex));
		},
		FetchEvent<Click>(btnTest) += [this](TouchEventArgs&&){
			YAssert(bool(pWndTest), "Null pointer found.");

			SwitchVisible(*pWndTest);
		},
		FetchEvent<Click>(btnOK) += [this](TouchEventArgs&&){
			if(fbMain.IsSelected())
			{
				const auto& path(fbMain.GetPath());
				const string& s(path.GetNativeString());

				if(!IO::ValidatePath(s) && ufexists(s))
				{
					if(GetEntryType(s) == EnrtySpace::Text
						&& !chkHex.IsTicked())
					// TODO: use g++ 4.7 later;
					//	SetShellTo(make_shared<ShlTextReader>(path));
						SetShellTo(share_raw(new ShlTextReader(path)));
					//	SetShellToNew<ShlTextReader>(path);
					else
					// TODO: use g++ 4.7 later;
					//	SetShellTo(make_shared<ShlHexBrowser>(path));
						SetShellTo(share_raw(new ShlHexBrowser(path)));
					//	SetShellToNew<ShlHexBrowser>(path);
				}
			}
		},
		FetchEvent<Click>(chkFPS) += [this](TouchEventArgs&&){
			SetInvalidationOf(GetDesktopDown());
		},
		FetchEvent<Click>(chkHex) += [this](TouchEventArgs&&){
			Enable(btnOK, CheckReaderEnability(fbMain, chkHex));
			SetInvalidationOf(GetDesktopDown());
		}
	);
	RequestFocusCascade(fbMain),
	SetInvalidationOf(dsk_up),
	SetInvalidationOf(dsk_dn);
	// FIXME: memory leaks;
	mhMain += *(ynew Menu(Rect::Empty, GenerateList(u"A:MenuItem"), 1u)),
	mhMain += *(ynew Menu(Rect::Empty, GenerateList(u"B:MenuItem"), 2u));
	mhMain[1u] += make_pair(1u, &mhMain[2u]);
	ResizeForContent(mhMain[2u]);
}


ShlExplorer::TFormTest::TFormTest()
	: Form(Rect(10, 40, 228, 100), shared_ptr<Image>()),
	btnEnterTest(Rect(4, 4, 146, 22)), /*FetchImage(6)*/
	btnMenuTest(Rect(152, 4, 60, 22)),
	btnShowWindow(Rect(8, 32, 104, 22)),
	btnPrevBackground(Rect(45, 64, 30, 22)),
	btnNextBackground(Rect(95, 64, 30, 22))
//Rect(120, 32, 96, 22)
{
	AddWidgets(*this, btnEnterTest, btnMenuTest, btnShowWindow,
		btnPrevBackground, btnNextBackground),
	tie(Background, btnEnterTest.Text, btnEnterTest.HorizontalAlignment,
		btnEnterTest.VerticalAlignment, btnMenuTest.Text, btnShowWindow.Text,
		btnShowWindow.HorizontalAlignment, btnShowWindow.VerticalAlignment,
		btnPrevBackground.Text, btnNextBackground.Text) = forward_as_tuple(
		SolidBrush(Color(248, 248, 120)), u"边界测试",
		TextAlignment::Right, TextAlignment::Up, u"菜单测试", u"显示/隐藏窗口",
		TextAlignment::Left, TextAlignment::Down, u"<<", u">>"),
	SetInvalidationOf(*this);

	static int up_i(1);

	yunseq(
		FetchEvent<TouchMove>(*this) += OnTouchMove_Dragging,
		FetchEvent<Enter>(btnEnterTest) += [](TouchEventArgs&& e){
			char str[20];

			std::sprintf(str, "Enter: (%d, %d)", e.GetX(), e.GetY());

			auto& btn(ystdex::polymorphic_downcast<Button&>(e.GetSender()));

			btn.Text = str;
			Invalidate(btn);
		},
		FetchEvent<Leave>(btnEnterTest) += [](TouchEventArgs&& e){
			char str[20];

			std::sprintf(str, "Leave: (%d, %d)", e.GetX(), e.GetY());

			auto& btn(ystdex::polymorphic_downcast<Button&>(e.GetSender()));

			btn.Text = str;
			Invalidate(btn);
		},
		FetchEvent<Click>(btnMenuTest) +=[this](TouchEventArgs&&){
			static int t;

			auto& mhMain(FetchShell<ShlExplorer>().mhMain);
			auto& mnu(mhMain[1u]);
			auto& lst(mnu.GetList());

			if(mhMain.IsShowing(1u))
			{
				if(lst.size() > 4)
					lst.clear();

				char stra[4];

				std::sprintf(stra, "%d", t);
				lst.push_back(string("TMI") + stra);
			}
			else
			{
				SetLocationOf(mnu, Point(
					btnMenuTest.GetX() - btnMenuTest.GetWidth(),
					btnMenuTest.GetY() + btnMenuTest.GetHeight()));
				mnu.SetWidth(btnMenuTest.GetWidth() + 20);
				lst.push_back(u"TestMenuItem1");
				mhMain.Show(1u);
			}
			ResizeForContent(mnu);
			Invalidate(mnu);
			++t;
		},
		FetchEvent<Click>(btnShowWindow) += OnClick_ShowWindow,
		FetchEvent<Click>(btnPrevBackground) += [this](TouchEventArgs&&){
			auto& shl(FetchShell<ShlExplorer>());
			auto& dsk_up(shl.GetDesktopUp());
			auto& dsk_dn(shl.GetDesktopDown());

			if(up_i > 1)
			{
				--up_i;
				Enable(btnNextBackground);
			}
			if(up_i == 1)
				Enable(btnPrevBackground, false);
			dsk_up.Background = ImageBrush(FetchImage(up_i));
			dsk_dn.Background = ImageBrush(FetchImage(up_i + 1));
			SetInvalidationOf(dsk_up);
			SetInvalidationOf(dsk_dn);
		},
		FetchEvent<Click>(btnNextBackground) += [this](TouchEventArgs&&){
			auto& shl(FetchShell<ShlExplorer>());
			auto& dsk_up(shl.GetDesktopUp());
			auto& dsk_dn(shl.GetDesktopDown());

			if(up_i < 5)
			{
				++up_i;
				Enable(btnPrevBackground);
			}
			if(up_i == 5)
				Enable(btnNextBackground, false);
			dsk_up.Background = ImageBrush(FetchImage(up_i));
			dsk_dn.Background = ImageBrush(FetchImage(up_i + 1));
			SetInvalidationOf(dsk_up);
			SetInvalidationOf(dsk_dn);
		}
	);
	Enable(btnPrevBackground, false);
}

ShlExplorer::TFormExtra::TFormExtra()
	: Form(Rect(5, 60, 208, 120), shared_ptr<Image>()), /*FetchImage(7)*/
	btnDragTest(Rect(13, 15, 184, 22)),
	btnTestEx(Rect(13, 52, 168, 22)),
	btnClose(Rect(13, 82, 60, 22)),
	btnExit(Rect(83, 82, 60, 22))
{
	AddWidgets(*this, btnDragTest, btnTestEx, btnClose, btnExit),
	yunseq(
		btnDragTest.Text = u"测试拖放控件",
		btnDragTest.HorizontalAlignment = TextAlignment::Left,
		btnTestEx.Text = u"附加测试",
		btnClose.Text = u"关闭",
		btnExit.Text = u"退出",
		Background = SolidBrush(Color(248, 120, 120)),
		//	btnDragTest.Enabled = false,
		btnClose.Background = SolidBrush(Color(176, 184, 192))
	),
	SetInvalidationOf(*this);
	yunseq(
		FetchEvent<TouchDown>(*this) += [this](TouchEventArgs&& e){
			Background = SolidBrush(GenerateRandomColor());
			SetInvalidationOf(*this);
			if(e.Strategy == RoutedEventArgs::Direct)
				e.Handled = true;
		},
		FetchEvent<TouchMove>(*this) += OnTouchMove_Dragging,
		FetchEvent<Move>(btnDragTest) += [this](UIEventArgs&&){
			char sloc[30];

			std::sprintf(sloc, "(%d, %d);", btnDragTest.GetX(),
				btnDragTest.GetY());
			btnDragTest.Text = sloc;
			Invalidate(btnDragTest);
		},
		FetchEvent<TouchDown>(btnDragTest) += [this](TouchEventArgs&&){
#ifndef YCL_MINGW32
			char strMemory[40];
			struct mallinfo t(mallinfo());

			std::sprintf(strMemory, "%d,%d,%d,%d,%d;",
				t.arena, t.ordblks, t.uordblks, t.fordblks, t.keepcost);
			auto& lblA(FetchShell<ShlExplorer>().lblA);

			lblA.Text = strMemory;
			Invalidate(lblA);
#endif
		},
		FetchEvent<TouchMove>(btnDragTest) += OnTouchMove_Dragging,
		FetchEvent<Click>(btnDragTest) += [this](TouchEventArgs&&){
			yunseq(
				btnDragTest.ForeColor = GenerateRandomColor(),
				btnClose.ForeColor = GenerateRandomColor()
			);
			Invalidate(*this);
		//	Enable(btnClose);
		},
	//	FetchEvent<Click>(btnTestEx) += [this](TouchEventArgs&& e){
	//	},
		FetchEvent<KeyPress>(btnDragTest) += [](KeyEventArgs&& e){
			const auto& k(e.GetKeys());
			char strt[100];
			auto& lbl(polymorphic_downcast<Label&>(e.GetSender()));

			lbl.SetTransparent(!lbl.IsTransparent());
			std::sprintf(strt, "%lu;\n", k.to_ulong());
			lbl.Text = strt;
			Invalidate(lbl);
		},
		FetchEvent<Click>(btnClose) += [this](TouchEventArgs&&){
			Hide(*this);
		},
		FetchEvent<Click>(btnExit) += [](TouchEventArgs&&){
			YSL_ PostQuitMessage(0);
		}
	);
}


void
ShlExplorer::OnPaint()
{
	// NOTE: overwriting member function OnInput using SM_TASK is also valid due
	//	to the SM_INPUT message is sent continuously, but with less efficiency.
	auto& dsk_dn(GetDesktopDown());

	if(chkFPS.IsTicked())
	{
		using namespace ColorSpace;

		const u32 t(fpsCounter.Refresh());

		if(t != 0)
		{
			auto& g(dsk_dn.GetContext());
			yconstexpr Rect r(0, 172, 80, 20);
			char strt[20];

			std::sprintf(strt, "FPS: %u.%03u", t / 1000, t % 1000);
			FillRect(g, r, Blue);
			DrawText(g, r, strt, DefaultMargin, White);
			bUpdateDown = true;
		}
	}
}

IWidget*
ShlExplorer::GetBoundControlPtr(const KeyInput& k)
{
	if(k.count() == 1)
	{
		if(k[YCL_KEY(X)])
			return &btnTest;
		if(k[YCL_KEY(A)])
			return &btnOK;
	}
	return nullptr;
}

void
ShlExplorer::OnClick_ShowWindow(TouchEventArgs&&)
{
	auto& pWnd(FetchShell<ShlExplorer>().pWndExtra);

	YAssert(bool(pWnd), "Null pointer found.");

	SwitchVisible(*pWnd);
}

YSL_END_NAMESPACE(YReader)

