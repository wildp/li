#include <iostream>

#include "l1.h"

lx::l1::expr_t i(lx::integer_t i)
{
    return std::make_unique<lx::l1::integer>(i);
}

int main()
{
    using namespace lx::l1;
    
    lx::l1_expr expr{ 
        std::make_unique<seq>(
            std::make_unique<assign>("l2", i(0)),
            std::make_unique<while_do>(
                std::make_unique<op_ge>(
                    std::make_unique<deref>("l1"),
                    i(1)
                ),
                std::make_unique<seq>(
                    std::make_unique<assign>("l2",
                        std::make_unique<op_add>(
                            std::make_unique<deref>("l2"),
                            std::make_unique<deref>("l1")
                        )
                    ),
                    std::make_unique<assign>("l1",
                        std::make_unique<op_add>(
                            std::make_unique<deref>("l1"),
                            i(-1)
                        )  
                    )
                )
            )
        )
        , store{ {"l1", 1000000}, {"l2", 0} } };


expr.eval();

/*
    auto prev_state = expr.get_state();
    std::cout << "Initial: " <<  prev_state.deref(loc{"l1"}) << ' ' << prev_state.deref(loc{"l2"}) << '\n';
    while (!expr.has_terminated())
    {cd 
        bool change{ false };
        while (!change && !expr.has_terminated())
        {
            expr.step();
            auto s = expr.get_state();
            change = (s != prev_state);
            if (change)
                prev_state = s;
        }
        std::cout << prev_state.deref(loc{"l1"}) << ' ' << prev_state.deref(loc{"l2"}) << '\n';
    }
    */
    //std::cout << "\n\nFinal: " <<  prev_state.deref(loc{"l1"}) << ' ' << prev_state.deref(loc{"l2"}) << '\n';
    
    auto state = expr.get_state();

    std::cout << "\n\nFinal: " <<  state.deref(loc{"l2"}) << '\n'; 
    
    return 0;
}