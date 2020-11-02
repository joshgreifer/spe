//
// Created by josh on 30/10/2020.
//

#ifndef SPE_GRAPH_H
#define SPE_GRAPH_H
#pragma once
#include <iostream>
#include "new_processor.h"
#include "../eng/scheduler.h"
#include "../eng/dag.h"

namespace sel
{
    namespace eng7
    {
        namespace proc {

            class processor_sequence :
                    public processor,
                    protected std::vector<processor *>

            {
            public:

                processor_sequence() = default;


                void process() final {
                    for (auto proc : *this) {
                        proc->process();
                    }
                }

                void init(eng::schedule *context) final {
                    for (auto proc : *this) {
                        proc->init(context);
                    }
                }

                void term(eng::schedule *context) final {
                    for (auto proc : *this) {
                        proc->term(context);
                    }
                }
            };
            /*
                Processor which contains a directed acyclic graph of sub processors
                It is constructed with no input or output ports

                They get created by:
                AddInputProc (which makes all the added processor's input ports available as input ports of the containing processor)
                AddOutputProc (which makes all the added processor's output ports available as output ports of the containing processor)

                A Subclass of processor_dag will use the above methods, and AddProc(), ConnectProcs() to wire itself up at construction time

            */
            class processor_dag  : public processor_sequence
            {
                dag<processor>dag_;		// for sorting

//                bool is_added(processor *proc) {
//                    return std::find(begin(), end(), proc) != end();
//                }

                void add_edge(processor &from, processor &to)
                {
                    dag_.add_edge(&from, &to);
                    clear();
                    if (!dag_.topological_sort(*this))
                        throw sp_ex_cycle();
                }

//                bool add_node(processor& proc)
//                {
//                    if (!is_added(&proc)) {
//                        push_back(&proc);
//                        return true;
//                    }
//                    return false;
//
//                }
//
//                // If node is a DAG, add its sub-processors
//                bool add_node(processor_dag& proc)
//                {
//                    bool ret = true;
//                    //for (ConnectableProcessor * p : proc)
//                    //	ret &= add_node(*p);
//                    for (auto& edge : proc.dag_.edges)
//                        add_edge(*edge.first, *edge.second);
//                    return ret;
//
//                }

            public:

                processor_dag() = default;

                template<typename FROM_PROC, typename TO_PROC, size_t from_pin = 0, size_t to_pin = 0>
                void connect_procs(FROM_PROC& from, TO_PROC& to)
                {
                    add_edge(from, to);
                    from.template connect_to<TO_PROC,from_pin, to_pin>(to);
                }


            };


            /*
             * Graph of processors
             *
             */
            class processor_graph
            {

                using fiber=processor_dag;
                std::map<eng::semaphore*, fiber*> sem_map;

                std::map<processor*, fiber*> proc_map;
                eng::scheduler& s_;
            public:

                processor_graph(eng::scheduler& s = eng::scheduler::get()) : s_(s) {}

                template<class FROM_PROC, class TO_PROC, size_t from_pin = 0, size_t to_pin = 0>
                void connect(FROM_PROC& from, TO_PROC& to)
                {
                    // if 'from' is rate-triggering, create a new fiber,  and do new_fiber.connect_procs()
                    // if 'from' is not rate-triggering,  find the fiber containing it.  If found, fiber_containing_from.connect_procs()
                    //		if not found, look for 'to' proc,  and do fiber_containing_to.connect_procs()
                    //			if *still* not found, create a new fiber,  and do new_fiber.connect_procs()

                        auto sem = dynamic_cast<eng::semaphore *>(&from);
                        fiber *fib = nullptr;
                        if (sem)  // 'from' is rate-triggering
                        {
                            fib = sem_map[sem];
                            if (!fib) {
                                fib = sem_map[sem] = new fiber;
                                // create a new schedule
                                s_.add(sem, *fib);
                            }
                            // 'to' belongs to same fiber as 'from'

                            proc_map[&to] = fib;

                        } else // 'from' is not rate-triggering, get its fiber
                        {
                            fib = proc_map[&from];
                            if (!fib) // no fiber, see if 'to''s fiber is registered
                            {
      
                                    fib = proc_map[&to];

                                if (!fib) // 'to' is not registered either, create a new fiber for it
                                {
                                    fib = new fiber;

                                }
                                proc_map[&from] = fib; // register fiber
                            }


                        }

                    fib->connect_procs(from, to);

                }
            };

        } // proc

    }
}

#endif //SPE_GRAPH_H
