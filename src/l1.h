#ifndef LANG_L1_H
#define LANG_L1_H

#include <utility>
#include <memory>
#include <string>
#include <variant>
#include <optional>
#include <stdexcept>
#include <exception>
#include <unordered_map>
#include <initializer_list>

#include "lang.hpp"

namespace lx::l1
{
    class expression;
    class value;

    enum class type {
        integer,
        boolean,
        unit
    };
    
    struct loc;
    class store;

    typedef std::unique_ptr<expression> expr_t;
    typedef std::optional<expr_t> exprreturn_t;

    class stuck_error : public std::runtime_error
    {
    public:
        stuck_error();
    };

    class type_error : public std::runtime_error
    {
    public:
        type_error();
        // TODO: variable argument constructor for current expression,
        // and any predicates required for rule
    };
    
    class location_error : public std::runtime_error
    {
    public:
        location_error(std::string expression, loc l);
    };

    class expression
    {
    public:
        virtual exprreturn_t step(store& s) = 0;
        virtual const type check(const store& s) = 0;
        virtual expr_t copy() = 0;
        virtual ~expression() {};
    };

    struct loc
    {
        const std::string id;
        loc(const std::string s) : id{ s } {}
    };

    class store
    {
        std::unordered_map<std::string, integer_t> s;
    public:
        store() = default;
        store(const std::string k, integer_t v);
        store(std::initializer_list<std::pair<const std::string, integer_t>> state);
        void assign(loc l, integer_t v);
        bool contains(loc l) const;
        integer_t deref(loc l) const;
        friend bool operator==(const store& lhs, const store& rhs);
    };

    class value : public expression
    {
    };

    class boolean : public value
    {
        bool v;
    public:
        boolean(bool v);
        boolean(const boolean&) = default;
        exprreturn_t step(store &s) override;
        const type check(const store& s) override;
        expr_t copy() override;
        bool get() const noexcept { return v; }
    };

    class integer : public value
    {
        integer_t v;
    public:
        integer(integer_t v);
        integer(const integer&) = default;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
        integer_t get() const noexcept { return v; }
    };

    class skip : public value
    {
    public:
        skip();
        skip(const skip&) = default;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
    };

    class operation: public expression
    {
    protected:
        expr_t lhs;
        expr_t rhs;
        operation(expr_t&& e1, expr_t&& e2);
    };

    class op_add : public operation
    {
    public:
        op_add(expr_t&& e1, expr_t&& e2);
        op_add(const op_add&) = delete;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
    };

    class op_ge: public operation
    {
    public:
        op_ge(expr_t&& e1, expr_t&& e2);
        op_ge(const op_ge&) = delete;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
    };

    class deref: public expression
    {
        loc l;
    public:
        deref(loc l);
        deref(const std::string l_id);
        deref(const deref&) = delete;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
    };

    class assign: public expression
    {
        loc l;
        expr_t e;
    public:
        assign(loc l, expr_t&& e);
        assign(const std::string l_id, expr_t&& e);
        assign(const assign&) = delete;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
    };

    class seq: public expression
    {
        expr_t e1;
        expr_t e2;
    public:
        seq(expr_t&& e1, expr_t&& e2);
        seq(const seq&) = delete;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
    };

    class if_then_else: public expression
    {
        expr_t e1;
        expr_t e2;
        expr_t e3;
    public:
        if_then_else(expr_t&& e1, expr_t&& e2, expr_t&& e3);
        if_then_else(const if_then_else&) = delete;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
    };

    class while_do: public expression
    {
        expr_t e1;
        expr_t e2;
    public:
        while_do(expr_t&& e1, expr_t&& e2);
        while_do(const while_do&) = delete;
        exprreturn_t step(store& s) override;
        const type check(const store& s) override;
        expr_t copy() override;
    };

    typedef std::variant<integer, boolean, skip> val;
}

namespace lx
{
    class l1_expr
    {
        l1::expr_t e;
        l1::store s;
    public:
        l1_expr(l1::expr_t&& e, l1::store&& s);
        bool type_check();
        void step();
        l1::val eval();
        const l1::store get_state() const;
        bool has_terminated();
    };
}

#endif // LANG_L1_H