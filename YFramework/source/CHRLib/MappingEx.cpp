﻿/*
	© 2012-2015 FrankHB.

	This file is part of the YSLib project, and may only be used,
	modified, and distributed under the terms of the YSLib project
	license, LICENSE.TXT.  By continuing to use, modify, or distribute
	this file you indicate that you have read the license and
	understand and accept it fully.
*/

/*!	\file MappingEx.cpp
\ingroup CHRLib
\brief 附加编码映射。
\version r83
\author FrankHB <frankhb1989@gmail.com>
\since build 324
\par 创建时间:
	2012-07-09 09:04:43 +0800
\par 修改时间:
	2015-10-14 01:03 +0800
\par 文本编码:
	UTF-8
\par 模块名称:
	CHRLib::MappingEx
*/


#include "CHRLib/YModules.h"
#include YFM_CHRLib_MappingEx

namespace CHRLib
{

using namespace CharSet;

namespace
{

//! \since build 641
template<byte*& _vCPMapPtr, size_t _vMax>
char16_t
dbcs_lkp(byte seq0, byte seq1)
{
	const auto idx(size_t(seq0 << 8U | seq1));

	return YB_LIKELY(idx < 0xFF7E)
		? reinterpret_cast<const char16_t*>(_vCPMapPtr + 0x0100)[idx] : 0;
}

} // unnamed namespacce;

#if !CHRLib_NoDynamicMapping

#if 0
byte* cp17;
#endif
byte* cp113;
#if 0
byte* cp2026;
#endif

#endif

char16_t(*cp113_lkp)(byte, byte) = dbcs_lkp<CHRLib::cp113, 0xFF7E>;

} // namespace CHRLib;

