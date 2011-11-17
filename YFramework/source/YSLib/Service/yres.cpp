﻿/*
	Copyright (C) by Franksoft 2009 - 2011.

	This file is part of the YSLib project, and may only be used,
	modified, and distributed under the terms of the YSLib project
	license, LICENSE.TXT.  By continuing to use, modify, or distribute
	this file you indicate that you have read the license and
	understand and accept it fully.
*/

/*!	\file yres.cpp
\ingroup Service
\brief 应用程序资源管理模块。
\version r1238;
\author FrankHB<frankhb1989@gmail.com>
\par 创建时间:
	2009-12-22 17:28:28 +0800;
\par 修改时间:
	2011-11-05 11:24 +0800;
\par 字符集:
	UTF-8;
\par 模块名称:
	YSLib::Service::YResource;
*/


#include "YSLib/Service/yres.h"
#include "YSLib/Core/yshell.h"

YSL_BEGIN

YSL_BEGIN_NAMESPACE(Drawing)

Image::Image(ConstBitmapPtr s, SDst w, SDst h)
	: BitmapBuffer(s, w, h)
{}

void
Image::SetImage(ConstBitmapPtr s, SDst w, SDst h)
{
	SetSize(w, h);
	if(pBuffer && s)
		mmbcpy(pBuffer, s, GetSizeOfBuffer());
}

YSL_END_NAMESPACE(Drawing)

YSL_END
