#include "l1.h"

#include <typeinfo>
#include <iostream>

#include "lang.hpp"


namespace lx::l1 {

    stuck_error::stuck_error() : runtime_error("stuck error") {}
    type_error::type_error() : runtime_error("stuck error") {}
    location_error::location_error(std::string expression, loc l) : 
        runtime_error("location error: " + expression + ": location " + l.id + " does not exist in store.") {}

    template<typename T> requires std::is_base_of_v<expression, T>
    inline const bool has_type(expr_t& ptr)
    {
        return typeid(*ptr.get()) == typeid(T);
    }

    inline const bool is_value(expr_t& ptr)
    {
        return has_type<integer>(ptr) || has_type<boolean>(ptr) || has_type<skip>(ptr); 
    }

    inline void evalstep(expr_t& e, store& s)
    {
        exprreturn_t expr{ e->step(s) };
        if (expr)
            e = std::move(expr.value());
    }


    /* Store */

    store::store(const std::string k, integer_t v) : s{} { s.emplace(k, v); };
    store::store(std::initializer_list<std::pair<const std::string, integer_t>> state) : s{ state } {}
    void store::assign(loc l, integer_t v) { s.at(l.id) = v; }
    bool store::contains(loc l) const { return s.contains(l.id); }
    integer_t store::deref(loc l) const { return s.at(l.id); }
    bool operator==(const store& lhs, const store& rhs) { return lhs.s == rhs.s; }

    /* L1 : Constructors */

    boolean::boolean(bool v) : v{ v } {}
    integer::integer(integer_t v) : v{ v } {}
    skip::skip() {}

    operation::operation(expr_t&& e1, expr_t&& e2) : lhs(std::move(e1)), rhs(std::move(e2)) {}
    op_add::op_add(expr_t&& e1, expr_t&& e2) : operation(std::move(e1), std::move(e2)) {}
    op_ge::op_ge(expr_t&& e1, expr_t&& e2) : operation(std::move(e1), std::move(e2)) {}

    deref::deref(loc l) : l{ l } {}
    deref::deref(const std::string l_id) : l{ l_id } {}
    assign::assign(loc l, expr_t&& e) : l{ l }, e{ std::move(e) } {}
    assign::assign(const std::string l_id, expr_t&& e) : l{ l_id }, e{ std::move(e) } {}

    seq::seq(expr_t&& e1, expr_t&& e2) : e1{ std::move(e1) }, e2{ std::move(e2) } {}
    if_then_else::if_then_else(expr_t&& e1, expr_t&& e2, expr_t&& e3) : e1{ std::move(e1) }, e2{ std::move(e2) }, e3{ std::move(e3) } {}
    while_do::while_do(expr_t&& e1, expr_t&& e2) : e1{ std::move(e1) }, e2{ std::move(e2) } {}

    expr_t boolean::copy() { return std::make_unique<boolean>(v); }
    expr_t integer::copy() { return std::make_unique<integer>(v); }
    expr_t skip::copy() { return std::make_unique<skip>(); }

    expr_t op_add::copy() { return std::make_unique<op_add>(lhs->copy(), rhs->copy()); }
    expr_t op_ge::copy() { return std::make_unique<op_ge>(lhs->copy(), rhs->copy()); }

    expr_t deref::copy() { return std::make_unique<deref>(loc{ l }); }
    expr_t assign::copy() { return std::make_unique<assign>(loc{ l }, e->copy()); }
    
    expr_t seq::copy() { return std::make_unique<seq>(e1->copy(), e2->copy()); }
    expr_t if_then_else::copy() { return std::make_unique<if_then_else>(e1->copy(), e2->copy(), e3->copy()); }
    expr_t while_do::copy() { return std::make_unique<while_do>(e1->copy(), e2->copy()); }


    /* L1 : Execution */

    exprreturn_t boolean::step(store& s) { return exprreturn_t{}; }
    exprreturn_t integer::step(store& s) { return exprreturn_t{}; }
    exprreturn_t skip::step(store& s) { return exprreturn_t{}; }

    exprreturn_t op_add::step(store& s)
    {
        if (has_type<integer>(lhs) && has_type<integer>(rhs))
            return exprreturn_t{ std::make_unique<integer>(dynamic_cast<integer*>(lhs.get())->get() + dynamic_cast<integer*>(rhs.get())->get()) }; //op+ rule
        else if (!is_value(lhs))
            evalstep(lhs, s); // op1 rule
        else /* if (!is_value(rhs)) */
            evalstep(rhs, s); // op2 rule
        return exprreturn_t(); 
    }

    exprreturn_t op_ge::step(store& s)
    {
        if (has_type<integer>(lhs) && has_type<integer>(rhs))
            return exprreturn_t{ std::make_unique<boolean>(dynamic_cast<integer*>(lhs.get())->get() >= dynamic_cast<integer*>(rhs.get())->get()) }; // op>= rule
        else if (!is_value(lhs))
            evalstep(lhs, s); // op1 rule
        else /* if (!is_value(rhs)) */
            evalstep(rhs, s); // op2 rule
        return exprreturn_t{}; 
    }

    exprreturn_t deref::step(store& s)
    {
        return exprreturn_t{ std::make_unique<integer>(s.deref(l)) }; // deref rule
    }

    exprreturn_t assign::step(store& s)
    {
        if (has_type<integer>(e))
        {
            s.assign(l, dynamic_cast<integer*>(e.get())->get()); // assign1 rule
            return exprreturn_t{ std::make_unique<skip>() }; // assign1 rule (continued)
        }
        else
            evalstep(e, s); // assign2 rule
        return exprreturn_t{};
    }

    exprreturn_t seq::step(store& s)
    {
        if (has_type<skip>(e1))
            return exprreturn_t{ std::move(e2) }; // seq1 rule
        else /* if (!is_value(e1)) */
            evalstep(e1, s); // seq2 rule
        return exprreturn_t{};
    }

    exprreturn_t if_then_else::step(store& s)
    {
        if (has_type<boolean>(e1))
        {
            if (dynamic_cast<boolean*>(e1.get())->get())
                return exprreturn_t{ std::move(e2) }; // if1 rule
            else
                return exprreturn_t{ std::move(e3) }; // if2 rule
        }
        else /* if (!is_value(e1)) */
            evalstep(e1, s); // if3 rule
        return exprreturn_t{};
    }

    exprreturn_t while_do::step(store& s)
    {
        auto e1_cpy{ e1->copy() };
        auto e2_cpy{ e2->copy() };
        return exprreturn_t{ std::make_unique<if_then_else>(std::move(e1_cpy), std::make_unique<seq>(std::move(e2_cpy),
           std::make_unique<while_do>(std::move(e1), std::move(e2))), std::make_unique<skip>()) }; // while rule
    }

    /* L1 : Non-compliant Execution */

    val_t boolean::eval_nc(store& s) const { return val_t{ v }; }
    val_t integer::eval_nc(store& s) const { return val_t{ v }; }
    val_t skip::eval_nc(store& s) const { return val_t{}; }

    val_t op_add::eval_nc(store& s) const
    {
        return val_t{ std::get<integer_t>(lhs->eval_nc(s)) + std::get<integer_t>(rhs->eval_nc(s)) }; 
    }

    val_t op_ge::eval_nc(store& s) const
    {
        return val_t{ std::get<integer_t>(lhs->eval_nc(s)) >= std::get<integer_t>(rhs->eval_nc(s)) }; 
    }

    val_t deref::eval_nc(store& s) const
    {
        return val_t{ s.deref(l) };
    }

    val_t assign::eval_nc(store& s) const
    {
        s.assign(l, std::get<integer_t>(e->eval_nc(s)));
        return val_t{}; 
    }

    val_t seq::eval_nc(store& s) const
    {
        e1->eval_nc(s);
        return e2->eval_nc(s);
    }

    val_t if_then_else::eval_nc(store& s) const
    {
        if (std::get<bool>(e1->eval_nc(s)))
            return e2->eval_nc(s);
        else
            return e3->eval_nc(s);
    }

    val_t while_do::eval_nc(store& s) const
    {
        while (std::get<bool>(e1->eval_nc(s)))
            e2->eval_nc(s);
        return val_t{};
    }

    /* L1 : Typing */

    const type integer::check(const store& s) { return type::integer; }
    const type boolean::check(const store& s) { return type::boolean; }
    const type skip::check(const store& s) { return type::unit; }

    const type op_add::check(const store& s)
    {
        if (lhs->check(s) == type::integer && rhs->check(s) == type::integer)
            return type::integer;
        else
            throw type_error();
    }

    const type op_ge::check(const store& s)
    {
        if (lhs->check(s) == type::integer && rhs->check(s) == type::integer)
            return type::boolean;
        else
            throw type_error();
    }

    const type deref::check(const store& s)
    {
        if (s.contains(l))
            return type::integer;
        else
            throw location_error("deref", l);
    }

    const type assign::check(const store& s)
    {
        if (s.contains(l))
        {
            if (e->check(s) == type::integer)
                return type::unit;
            else
                throw type_error();
        }
        else
            throw location_error("assign", l);
    }

    const type seq::check(const store& s)
    {
        if (e1->check(s) == type::unit)
            return e2->check(s);
        else
            throw type_error();
    }

    const type if_then_else::check(const store& s)
    {
        type t = e2->check(s);
        if (e1->check(s) == type::boolean && e3->check(s) == t)
            return t;
        else
            throw type_error();
    }

    const type while_do::check(const store& s)
    {
        if (e1->check(s) == type::boolean && e2->check(s) == type::unit)
            return type::unit;
        else
            throw type_error();
    }

}


namespace lx
{
    l1_expr::l1_expr(l1::expr_t&& e, l1::store&& s) : e{ std::move(e) }, s{ std::move(s) }
    {
        this->e->check(this->s);
    }

    void l1_expr::step()
    {
        evalstep(e, s);
    }
    
    l1::val_t l1_expr::raw_eval()
    {
        while (!l1::is_value(e))
            evalstep(e, s);

        if (l1::has_type<l1::integer>(e))
            return l1::val_t{ dynamic_cast<l1::integer*>(e.get())->get() };
        else if (l1::has_type<l1::boolean>(e))
            return l1::val_t{ dynamic_cast<l1::boolean*>(e.get())->get() };
        else
            return l1::val_t{};
    }  

    std::pair<const l1::val_t, const l1::store> l1_expr::eval()
    {
        auto rv = raw_eval();
        return {rv, s};
    }

    std::pair<const l1::val_t, const l1::store> l1_expr::non_compliant_eval()
    {
        auto ns = l1::store{ s };
        auto rv = e->eval_nc(ns);
        return {rv, ns};
    }  

    const l1::store l1_expr::get_state() const
    {
        return s;
    }

    bool l1_expr::has_terminated()
    {
        return l1::is_value(e);
    } 
}