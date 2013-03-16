﻿/*
	Copyright by FrankHB 2011 - 2013.

	This file is part of the YSLib project, and may only be used,
	modified, and distributed under the terms of the YSLib project
	license, LICENSE.TXT.  By continuing to use, modify, or distribute
	this file you indicate that you have read the license and
	understand and accept it fully.
*/

/*!	\file uicontx.h
\ingroup UI
\brief 样式无关的图形用户界面附加容器。
\version r317
\author FrankHB <frankhb1989@gmail.com>
\since build 192
\par 创建时间:
	2011-02-21 08:59:34 +0800
\par 修改时间:
	2013-03-13 13:15 +0800
\par 文本编码:
	UTF-8
\par 模块名称:
	YSLib::UI::UIContainerEx
*/


#ifndef YSL_INC_UI_uicontx_h_
#define YSL_INC_UI_uicontx_h_ 1

#include "ypanel.h"
#include "button.h"
#include "WidgetIteration.h"

YSL_BEGIN

YSL_BEGIN_NAMESPACE(UI)

/*!
\brief 对话框。
\since build 276
*/
class YF_API DialogBox : public Control
{
public:
	//! \since build 357
	typedef ystdex::subscriptive_iterator<DialogBox, IWidget> Iterator;

protected:
	/*!
	\brief 关闭按钮。
	\since build 302
	*/
	Thumb btnClose;

public:
	//! \since build 337
	explicit
	DialogBox(const Rect& = {});

	//! \since build 357
	DefWidgetMemberIteration(btnClose)
};


/*!
\brief 对话面板。
\since build 276
*/
class YF_API DialogPanel : public Panel
{
public:
	/*!
	\brief 关闭按钮。
	\since build 302
	*/
	Thumb btnClose;
	Button btnOK;

	//! \since build 337
	explicit
	DialogPanel(const Rect& = {});
};

YSL_END_NAMESPACE(UI)

YSL_END

#endif

