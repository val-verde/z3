/*++
Copyright (c) 2011 Microsoft Corporation

Module Name:

    char_decl_plugin.cpp

Abstract:

    char_plugin for unicode suppport

Author:

    Nikolaj Bjorner (nbjorner) 2021-01-26


--*/
#pragma once

#include "util/zstring.h"
#include "ast/char_decl_plugin.h"
#include "ast/ast_pp.h"

char_decl_plugin::char_decl_plugin(): 
    m_charc_sym("Char") {
}

char_decl_plugin::~char_decl_plugin() {
    m_manager->dec_ref(m_char);
}

func_decl* char_decl_plugin::mk_func_decl(decl_kind k, unsigned num_parameters, parameter const* parameters,
                        unsigned arity, sort* const* domain, sort* range) {
    ast_manager& m = *m_manager;
    std::stringstream msg;
    switch (k) {
    case OP_CHAR_LE:
        if (arity != 2)
            msg << "incorrect number of arguments passed. Expected 2, received " << arity;
        else if(domain[0] != m_char)
            msg << "incorrect first argument type " << mk_pp(domain[0], *m_manager);
        else if (domain[1] != m_char)
            msg << "incorrect second argument type " << mk_pp(domain[1], *m_manager);     
        else             
            return m.mk_func_decl(symbol("char.<="), arity, domain, m.mk_bool_sort(), func_decl_info(m_family_id, k, 0, nullptr));
        m.raise_exception(msg.str());

    case OP_CHAR_CONST:
        if (num_parameters != 1)
            msg << "incorrect number of parameters passed. Expected 1, recieved " << num_parameters;
        else if (arity != 0)
            msg << "incorrect number of arguments passed. Expected 1, received " << arity;
        else if (!parameters[0].is_int())
            msg << "integer parameter expected";
        else if (parameters[0].get_int() < 0)
            msg << "non-negative parameter expected";
        else if (parameters[0].get_int() > static_cast<int>(zstring::unicode_max_char()))
            msg << "parameter expected within character range";
        else
            return m.mk_const_decl(m_charc_sym, m_char, func_decl_info(m_family_id, OP_CHAR_CONST, num_parameters, parameters));
        m.raise_exception(msg.str());
    default:
        UNREACHABLE();
    }    
    return nullptr;
}

void char_decl_plugin::set_manager(ast_manager * m, family_id id) {
    decl_plugin::set_manager(m, id);
    m_char = m->mk_sort(symbol("Unicode"), sort_info(m_family_id, CHAR_SORT, 0, nullptr));
    m->inc_ref(m_char);
}

void char_decl_plugin::get_op_names(svector<builtin_name>& op_names, symbol const& logic) {
    op_names.push_back(builtin_name("char.<=", OP_CHAR_LE));
}

void char_decl_plugin::get_sort_names(svector<builtin_name>& sort_names, symbol const& logic) {
    sort_names.push_back(builtin_name("Unicode", CHAR_SORT));
}

bool char_decl_plugin::is_value(app* e) const {
    return is_app_of(e, m_family_id, OP_CHAR_CONST);
}

bool char_decl_plugin::is_unique_value(app* e) const {
    return is_value(e);
}

bool char_decl_plugin::are_equal(app* a, app* b) const {
    return a == b;
}

bool char_decl_plugin::are_distinct(app* a, app* b) const {
    return 
        a != b && 
        is_app_of(a, m_family_id, OP_CHAR_CONST) && 
        is_app_of(b, m_family_id, OP_CHAR_CONST); 
}

app* char_decl_plugin::mk_char(unsigned u) {
    SASSERT(u <= zstring::unicode_max_char());
    parameter param(u);
    func_decl* f = m_manager->mk_const_decl(m_charc_sym, m_char, func_decl_info(m_family_id, OP_CHAR_CONST, 1, &param));
    return m_manager->mk_const(f);
}

expr* char_decl_plugin::get_some_value(sort* s) {
    return mk_char('A');
}

app* char_decl_plugin::mk_le(expr* a, expr* b) {
    expr_ref _ch1(a, *m_manager), _ch2(b, *m_manager);
    unsigned v1 = 0, v2 = 0;
    if (is_const_char(a, v1) && is_const_char(b, v2))
        return m_manager->mk_bool_val(v1 <= v2);
    expr* es[2] = { a, b };
    return m_manager->mk_app(m_family_id, OP_CHAR_LE, 2, es);    
}
