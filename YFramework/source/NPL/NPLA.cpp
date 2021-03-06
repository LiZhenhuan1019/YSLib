﻿/*
	© 2014-2018 FrankHB.

	This file is part of the YSLib project, and may only be used,
	modified, and distributed under the terms of the YSLib project
	license, LICENSE.TXT.  By continuing to use, modify, or distribute
	this file you indicate that you have read the license and
	understand and accept it fully.
*/

/*!	\file NPLA.cpp
\ingroup NPL
\brief NPLA 公共接口。
\version r1611
\author FrankHB <frankhb1989@gmail.com>
\since build 663
\par 创建时间:
	2016-01-07 10:32:45 +0800
\par 修改时间:
	2018-01-08 02:17 +0800
\par 文本编码:
	UTF-8
\par 模块名称:
	NPL::NPLA
*/


#include "NPL/YModules.h"
#include YFM_NPL_NPLA // for ystdex::value_or, ystdex::write,
//	ystdex::bad_any_cast, ystdex::unimplemented, ystdex::type_id, ystdex::quote,
//	ystdex::call_value_or, ystdex::begins_with, ystdex::sfmt, ystdex::ref,
//	ystdex::retry_on_cond, ystdex::type_info;
#include YFM_NPL_SContext
#include <ystdex/scope_guard.hpp> // for ystdex::share_guard;

using namespace YSLib;

namespace NPL
{

ValueNode
MapNPLALeafNode(const TermNode& term)
{
	return AsNode(string(),
		string(Deliteralize(ParseNPLANodeString(MapToValueNode(term)))));
}

ValueNode
TransformToSyntaxNode(const ValueNode& node, const string& name)
{
	ValueNode::Container con{AsIndexNode(size_t(), node.GetName())};
	const auto nested_call([&](const ValueNode& nd){
		con.emplace(TransformToSyntaxNode(nd, MakeIndex(con)));
	});

	if(node.empty())
	{
		if(const auto p = AccessPtr<NodeSequence>(node))
			for(auto& nd : *p)
				nested_call(nd);
		else
			con.emplace(NoContainer, MakeIndex(1),
				Literalize(ParseNPLANodeString(node)));
	}
	else
		for(auto& nd : node)
			nested_call(nd);
	return {std::move(con), name};
}

string
EscapeNodeLiteral(const ValueNode& node)
{
	return EscapeLiteral(Access<string>(node));
}

string
LiteralizeEscapeNodeLiteral(const ValueNode& node)
{
	return Literalize(EscapeNodeLiteral(node));
}

string
ParseNPLANodeString(const ValueNode& node)
{
	return ystdex::value_or(AccessPtr<string>(node));
}


string
DefaultGenerateIndent(size_t n)
{
	return string(n, '\t');
}

void
PrintIndent(std::ostream& os, IndentGenerator igen, size_t n)
{
	if(YB_LIKELY(n != 0))
		ystdex::write(os, igen(n));
}

void
PrintNode(std::ostream& os, const ValueNode& node, NodeToString node_to_str,
	IndentGenerator igen, size_t depth)
{
	PrintIndent(os, igen, depth);
	os << EscapeLiteral(node.GetName()) << ' ';
	if(PrintNodeString(os, node, node_to_str))
		return;
	os << '\n';
	if(node)
		TraverseNodeChildAndPrint(os, node, [&]{
			PrintIndent(os, igen, depth);
		}, [&](const ValueNode& nd){
			return PrintNodeString(os, nd, node_to_str);
		}, [&](const ValueNode& nd){
			return PrintNode(os, nd, node_to_str, igen, depth + 1);
		});
}

bool
PrintNodeString(std::ostream& os, const ValueNode& node,
	NodeToString node_to_str)
{
	TryRet(os << node_to_str(node) << '\n', true)
	CatchIgnore(ystdex::bad_any_cast&)
	return {};
}


namespace SXML
{

string
ConvertAttributeNodeString(const TermNode& term)
{
	switch(term.size())
	{
	default:
		YTraceDe(Warning, "Invalid term with more than 2 children found.");
		YB_ATTR_fallthrough;
	case 2:
		{
			auto i(term.begin());
			const auto& n(Access<string>(Deref(i)));

			return n + '=' + Access<string>(Deref(++i));
		}
		YB_ATTR_fallthrough;
	case 1:
		return Access<string>(Deref(term.begin()));
	case 0:
		break;
	}
	throw LoggedEvent("Invalid term with less than 1 children found.", Warning);
}

string
ConvertDocumentNode(const TermNode& term, IndentGenerator igen, size_t depth,
	ParseOption opt)
{
	if(term)
	{
		string res(ConvertStringNode(term));

		if(res.empty())
		{
			if(opt == ParseOption::String)
				throw LoggedEvent("Invalid non-string term found.");

			const auto& con(term.GetContainer());

			if(!con.empty())
				try
				{
					auto i(con.cbegin());
					const auto& str(Access<string>(Deref(i)));

					++i;
					if(str == "@")
					{
						for(; i != con.cend(); ++i)
							res += ' ' + ConvertAttributeNodeString(Deref(i));
						return res;
					}
					if(opt == ParseOption::Attribute)
						throw LoggedEvent("Invalid non-attribute term found.");
					if(str == "*PI*")
					{
						res = "<?";
						for(; i != con.cend(); ++i)
							res += string(Deliteralize(ConvertDocumentNode(
								Deref(i), igen, depth, ParseOption::String)))
								+ ' ';
						if(!res.empty())
							res.pop_back();
						return res + "?>";
					}
					if(str == "*ENTITY*" || str == "*NAMESPACES*")
					{
						if(opt == ParseOption::Strict)
							throw ystdex::unimplemented();
					}
					else if(str == "*COMMENT*")
						;
					else if(!str.empty())
					{
						const bool is_content(str != "*TOP*");
						string head('<' + str);
						bool nl{};

						if(YB_UNLIKELY(!is_content && depth > 0))
							YTraceDe(Warning, "Invalid *TOP* found.");
						if(i != con.end())
						{
							if(!Deref(i).empty()
								&& (i->begin())->Value == string("@"))
							{
								head += string(
									Deliteralize(ConvertDocumentNode(*i, igen,
									depth, ParseOption::Attribute)));
								if(++i == con.cend())
									return head + " />";
							}
							head += '>';
						}
						else
							return head + " />";
						for(; i != con.cend(); ++i)
						{
							nl = Deref(i).Value.type()
								!= ystdex::type_id<string>();
							if(nl)
								res += '\n' + igen(depth + size_t(is_content));
							else
								res += ' ';
							res += string(Deliteralize(ConvertDocumentNode(*i,
								igen, depth + size_t(is_content))));
						}
						if(res.size() > 1 && res.front() == ' ')
							res.erase(0, 1);
						if(!res.empty() && res.back() == ' ')
							res.pop_back();
						if(is_content)
						{
							if(nl)
								res += '\n' + igen(depth);
							return head + res + ystdex::quote(str, "</", '>');
						}
					}
					else
						throw LoggedEvent("Empty item found.", Warning);
				}
				CatchExpr(ystdex::bad_any_cast& e, YTraceDe(Warning,
					"Conversion failed: <%s> to <%s>.", e.from(), e.to()))
		}
		return res;
	}
	throw LoggedEvent("Empty term found.", Warning);
}

string
ConvertStringNode(const TermNode& term)
{
	return ystdex::call_value_or(EscapeXML, AccessPtr<string>(term));
}

void
PrintSyntaxNode(std::ostream& os, const TermNode& term, IndentGenerator igen,
	size_t depth)
{
	if(IsBranch(term))
		ystdex::write(os,
			ConvertDocumentNode(Deref(term.begin()), igen, depth), 1);
	os << std::flush;
}


ValueNode
MakeXMLDecl(const string& name, const string& ver, const string& enc,
	const string& sd)
{
	auto decl("version=\"" + ver + '"');

	if(!enc.empty())
		decl += " encoding=\"" + enc + '"';
	if(!sd.empty())
		decl += " standalone=\"" + sd + '"';
	return AsNodeLiteral(name, {{MakeIndex(0), "*PI*"}, {MakeIndex(1), "xml"},
		{MakeIndex(2), decl + ' '}});
}

ValueNode
MakeXMLDoc(const string& name, const string& ver, const string& enc,
	const string& sd)
{
	auto doc(MakeTop(name));

	doc.Add(MakeXMLDecl(MakeIndex(1), ver, enc, sd));
	return doc;
}

} // namespace SXML;


//! \since build 726
namespace
{

//! \since build 731
string
InitBadIdentifierExceptionString(string&& id, size_t n)
{
	return (n != 0 ? (n == 1 ? "Bad identifier: '" : "Duplicate identifier: '")
		: "Unknown identifier: '") + std::move(id) + "'.";
}

} // unnamed namespace;


ImplDeDtor(NPLException)


ImplDeDtor(ListReductionFailure)


ImplDeDtor(InvalidSyntax)


ImplDeDtor(ParameterMismatch)


ArityMismatch::ArityMismatch(size_t e, size_t r, RecordLevel lv)
	: ParameterMismatch(
	ystdex::sfmt("Arity mismatch: expected %zu, received %zu.", e, r), lv),
	expected(e), received(r)
{}
ImplDeDtor(ArityMismatch)


BadIdentifier::BadIdentifier(const char* id, size_t n, RecordLevel lv)
	: NPLException(InitBadIdentifierExceptionString(id, n), lv),
	p_identifier(make_shared<string>(id))
{}
BadIdentifier::BadIdentifier(string_view id, size_t n, RecordLevel lv)
	: NPLException(InitBadIdentifierExceptionString(string(id), n), lv),
	p_identifier(make_shared<string>(id))
{}
ImplDeDtor(BadIdentifier)


LexemeCategory
CategorizeBasicLexeme(string_view id) ynothrowv
{
	YAssertNonnull(id.data() && !id.empty());

	const auto c(CheckLiteral(id));

	if(c == '\'')
		return LexemeCategory::Code;
	if(c != char())
		return LexemeCategory::Data;
	return LexemeCategory::Symbol;
}

LexemeCategory
CategorizeLexeme(string_view id) ynothrowv
{
	const auto res(CategorizeBasicLexeme(id));

	return res == LexemeCategory::Symbol && IsNPLAExtendedLiteral(id)
		? LexemeCategory::Extended : res;
}

bool
IsNPLAExtendedLiteral(string_view id) ynothrowv
{
	YAssertNonnull(id.data() && !id.empty());

	const char f(id.front());

	return (id.size() > 1 && IsNPLAExtendedLiteralNonDigitPrefix(f)
		&& id.find_first_not_of("+-") != string_view::npos) || std::isdigit(f);
}


observer_ptr<const TokenValue>
TermToNamePtr(const TermNode& term)
{
	return AccessPtr<TokenValue>(term);
}

string
TermToString(const TermNode& term)
{
	if(const auto p = TermToNamePtr(term))
		return *p;
	return sfmt("#<unknown{%zu}:%s>", term.size(), term.Value.type().name());
}

void
TokenizeTerm(TermNode& term)
{
	for(auto& child : term)
		TokenizeTerm(child);
	if(const auto p = AccessPtr<string>(term))
		term.Value.emplace<TokenValue>(std::move(*p));
}


TermNode&
ReferenceTerm(TermNode& term)
{
	return ystdex::call_value_or(
		[&](const TermReference& term_ref) ynothrow -> TermNode&{
		return term_ref.get();
	}, AccessPtr<const TermReference>(term), term);
}
const TermNode&
ReferenceTerm(const TermNode& term)
{
	return ystdex::call_value_or(
		[&](const TermReference& term_ref) ynothrow -> const TermNode&{
		return term_ref.get();
	}, AccessPtr<TermReference>(term), term);
}


bool
CheckReducible(ReductionStatus status)
{
	if(status == ReductionStatus::Clean
		|| status == ReductionStatus::Retained)
		return {};
	if(YB_UNLIKELY(status != ReductionStatus::Retrying))
		YTraceDe(Warning, "Unexpected status found.");
	return true;
}


void
LiftTermOrRef(TermNode& term, TermNode& tm)
{
	if(const auto p = AccessPtr<const TermReference>(tm))
		LiftTermRef(term, p->get());
	else
		LiftTerm(term, tm);
}

void
LiftTermRefToSelf(TermNode& term)
{
	if(const auto p = AccessPtr<const TermReference>(term))
		LiftTermRef(term, p->get());
}

void
LiftToReference(TermNode& term, TermNode& tm)
{
	if(tm)
	{
		if(const auto p = AccessPtr<const TermReference>(tm))
			LiftTermRef(term, p->get());
		else if(!tm.Value.OwnsUnique())
			LiftTerm(term, tm);
		else
			throw NPLException(
				"Value of a temporary shall not be referenced.");
	}
	else
		ystdex::throw_invalid_construction();
}

void
LiftToSelf(TermNode& term)
{
	for(auto& child : term)
		LiftToSelf(child);
	LiftTermRefToSelf(term);
}

void
LiftToOther(TermNode& term, TermNode& tm)
{
	LiftTermRefToSelf(tm);
	LiftTerm(term, tm);
}


ReductionStatus
ReduceHeadEmptyList(TermNode& term) ynothrow
{
	if(term.size() > 1 && IsEmpty(Deref(term.begin())))
		RemoveHead(term);
	return ReductionStatus::Clean;
}

ReductionStatus
ReduceToList(TermNode& term) ynothrow
{
	return IsBranch(term) ? (void(RemoveHead(term)), ReductionStatus::Retained)
		: ReductionStatus::Clean;
}


//! \since build 798
namespace
{

yconstfn_relaxed bool
IsReserved(string_view id) ynothrowv
{
	YAssertNonnull(id.data());
	return ystdex::begins_with(id, "__");
}

observer_ptr<Environment>
RedirectToShared(string_view id, const shared_ptr<Environment>& p_shared)
{
	if(p_shared)
		return make_observer(get_raw(p_shared));
	// TODO: Use concrete semantic failure exception.
	throw NPLException(ystdex::sfmt("Invalid reference found for%s name '%s',"
		" probably due to invalid context access by dangling reference.",
		IsReserved(id) ? " reserved" : "", id.data()));
}

observer_ptr<const Environment>
RedirectParent(const ValueObject& parent, string_view id)
{
	const auto& tp(parent.type());

	if(tp == ystdex::type_id<EnvironmentList>())
		for(const auto& vo : parent.GetObject<EnvironmentList>())
		{
			auto p(RedirectParent(vo, id));

			if(p)
				return p;
		}
	if(tp == ystdex::type_id<observer_ptr<const Environment>>())
		return parent.GetObject<observer_ptr<const Environment>>();
	if(tp == ystdex::type_id<weak_ptr<Environment>>())
		return RedirectToShared(id,
			parent.GetObject<weak_ptr<Environment>>().lock());
	if(tp == ystdex::type_id<shared_ptr<Environment>>())
		return RedirectToShared(id,
			parent.GetObject<shared_ptr<Environment>>());
	return {};
}

} // unnamed namespace;

void
Environment::CheckParent(const ValueObject& vo)
{
	const auto& tp(vo.type());

	if(tp == ystdex::type_id<EnvironmentList>())
	{
		for(const auto& env : vo.GetObject<EnvironmentList>())
			CheckParent(env);
	}
	else if(YB_UNLIKELY(tp != ystdex::type_id<observer_ptr<const Environment>>()
		&& tp != ystdex::type_id<weak_ptr<Environment>>()
		&& tp != ystdex::type_id<shared_ptr<Environment>>()))
		ThrowForInvalidType(tp);
}

observer_ptr<const Environment>
Environment::DefaultRedirect(const Environment& env, string_view id)
{
	return RedirectParent(env.Parent, id);
}

observer_ptr<ValueNode>
Environment::DefaultResolve(const Environment& e, string_view id)
{
	YAssertNonnull(id.data());

	observer_ptr<ValueNode> p;
	auto env_ref(ystdex::ref<const Environment>(e));

	ystdex::retry_on_cond(
		[&](observer_ptr<const Environment> p_env) ynothrow -> bool{
		if(p_env)
		{
			env_ref = ystdex::ref(Deref(p_env));
			return true;
		}
		return {};
	}, [&, id]() -> observer_ptr<const Environment>{
		auto& env(env_ref.get());

		p = env.LookupName(id);
		return p ? observer_ptr<const Environment>() : env.Redirect(id);
	});
	return p;
}

void
Environment::Define(string_view id, ValueObject&& vo, bool forced)
{
	YAssertNonnull(id.data());
	if(forced)
		// XXX: Self overwriting is possible.
		swap(Bindings[id].Value, vo);
	else if(!Bindings.AddValue(id, std::move(vo)))
		throw BadIdentifier(id, 2);
}

observer_ptr<ValueNode>
Environment::LookupName(string_view id) const
{
	YAssertNonnull(id.data());
	return AccessNodePtr(Bindings, id);
}

void
Environment::Redefine(string_view id, ValueObject&& vo, bool forced)
{
	if(const auto p = LookupName(id))
		swap(p->Value, vo);
	else if(!forced)
		throw BadIdentifier(id, 0);
}

bool
Environment::Remove(string_view id, bool forced)
{
	YAssertNonnull(id.data());
	if(Bindings.Remove(id))
		return true;
	if(forced)
		return {};
	throw BadIdentifier(id, 0);
}

void
Environment::ThrowForInvalidType(const ystdex::type_info& tp)
{
	throw NPLException(ystdex::sfmt("Invalid environment type '%s' found.",
		tp.name()));
}


ContextNode::ContextNode(const ContextNode& ctx,
	shared_ptr<Environment>&& p_rec)
	: p_record([&]{
		if(p_rec)
			return std::move(p_rec);
		throw std::invalid_argument(
			"Invalid environment record pointer found.");
	}()),
	EvaluateLeaf(ctx.EvaluateLeaf), EvaluateList(ctx.EvaluateList),
	EvaluateLiteral(ctx.EvaluateLiteral), Trace(ctx.Trace)
{}
ContextNode::ContextNode(ContextNode&& ctx) ynothrow
	: ContextNode()
{
	swap(ctx, *this);
}

ReductionStatus
ContextNode::ApplyTail()
{
	// TODO: Avoid stack overflow when current action is called.
	YAssert(bool(Current), "No tail action found.");
	return LastStatus = Switch()();
}

void
ContextNode::Pop() ynothrow
{
	YAssert(!Delimited.empty(), "No continuation is delimited.");
	SetupTail(std::move(Delimited.front()));
	Delimited.pop_front();
}

void
ContextNode::Push(const Reducer& reducer)
{
	YAssert(Current, "No continuation can be captured.");
	Delimited.push_front(reducer);
	std::swap(Current, Delimited.front());
}
void
ContextNode::Push(Reducer&& reducer)
{
	YAssert(Current, "No continuation can be captured.");
	Delimited.push_front(std::move(reducer));
	std::swap(Current, Delimited.front());
}

ReductionStatus
ContextNode::Rewrite(Reducer reduce)
{
	SetupTail(reduce);
	// NOTE: Rewriting loop until no actions remain.
	return ystdex::retry_on_cond(std::bind(&ContextNode::Transit, this), [&]{
		return ApplyTail();
	});
}

ReductionStatus
ContextNode::RewriteGuarded(TermNode& term, Reducer reduce)
{
	const auto gd(Guard(term, *this));

	return Rewrite(reduce);
}

bool
ContextNode::Transit() ynothrow
{
	if(!Current)
	{
		if(!Delimited.empty())
			Pop();
		else
			return {};
	}
	return true;
}


ReductionStatus
RelayNext(ContextNode& ctx, ContextNode::Reducer&& cur,
	ContextNode::Reducer&& next)
{
	// TODO: Blocked. Use C++14 lambda initializers to implement move
	//	initialization.
	ctx.SetupTail(std::bind(
		[&](const ContextNode::Reducer& act, ContextNode::Reducer& act2){
		RelaySwitched(ctx, std::move(act2));
		return act();
	}, std::move(cur), std::move(next)));
	return ReductionStatus::Retrying;
}

} // namespace NPL;

