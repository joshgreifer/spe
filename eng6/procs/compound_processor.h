#pragma once
#include <iostream>
#include "../processor.h"
#include "../scheduler.h"
#include "../dag.h"

namespace sel
{
	namespace eng6 {
		namespace proc {

			class processor_sequence : 
			protected std::vector<ConnectableProcessor *>,
			public ConnectableProcessor, 
			public traceable<processor_sequence>

			{
			public:

				processor_sequence() = default;


				virtual std::ostream& trace(std::ostream& os) const override
				{
					for (auto proc : *this) {
						os << '\t'; proc->trace(os) << std::endl;
					}

					return os;
				}
				
				void freeze(void) override {
										
					for (auto proc :*this) {
						proc->freeze();
					}

				}
				
				void process() override {
					for (auto proc : *this) {
						proc->process();
					}
				}

				void init(schedule *context)  override {
					for (auto proc : *this) {
						proc->init(context);
					}
				}

				void term(schedule *context)  override {
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
				dag<ConnectableProcessor>dag_;		// for sorting

				bool is_added(ConnectableProcessor *proc) {
					return std::find(begin(), end(), proc) != end();
				}

			public:

				processor_dag() = default;


				void add_edge(ConnectableProcessor &from, ConnectableProcessor &to)
				{
					dag_.add_edge(&from, &to);
					clear();
					if (!dag_.topological_sort(*this))
						throw sp_ex_cycle();
				}

				bool add_node(ConnectableProcessor& proc)
				{
					if (!is_added(&proc)) {
						push_back(&proc);
						return true;
					}
					return false;

				}

				// If node is a DAG, add its sub-processors
				bool add_node(processor_dag& proc)
				{
					bool ret = true;
					//for (ConnectableProcessor * p : proc)
					//	ret &= add_node(*p);
					for (auto& edge : proc.dag_.edges)
						add_edge(*edge.first, *edge.second);
					return ret;

				}

			};

		/*
			Processor which contains a directed acyclic graph of sub processors
			It is constructed with no input or output ports

			They get created by:
			*	add_input_proc (which makes all the added processor's input ports available as input ports of the containing processor)
			*	add_output_proc (which makes all the added processor's output ports available as output ports of the containing processor)

			A Subclass of compound_processor will use the above methods, and 
			*	add_proc(), 
			*	connect_procs(),  
			*	connect_const(), 
			
			to wire itself up at construction time

			*/

			class compound_processor : public processor_dag,  public creatable<compound_processor>
			{
				using vec = std::vector<ConnectableProcessor *>;
				
				vec input_procs;
				vec output_procs;
				
			public:
				virtual const std::string type() const override { return "compound processor (dag)"; }

				compound_processor() = default;

				compound_processor(std::initializer_list<ConnectableProcessor *> procs)
				{
					// add to sequence - no need to topologically sort 
					for (ConnectableProcessor *proc: procs)
						vec::push_back(proc);

					// daisy-chain the procs
					// TODO: Handle add_input_proc() and add_output_proc()
					//
					auto p1 = vec::at(0);
					for (size_t i = 1; i < vec::size(); ++i)
					{
						auto p2 = vec::at(i);
						p1->ConnectTo(*p2);
						p1 = p2;
					}
				}

				explicit compound_processor(params& args)
				{
				}
				
				//auto& input(size_t proc_id = 0) 
				//{
				//	return *input_procs[proc_id];
				//}
				//auto& output(size_t proc_id = 0) 
				//{
				//	return *output_procs[proc_id];
				//}

				//// returns index of newly added input proc
				//size_t add_input_proc(ConnectableProcessor& proc, size_t from_port_id = PORTID_ALL)
				//{
				//	add_node(proc);
				//	input_procs.push_back(&proc);
				//	return input_procs.size() - 1;
				//}

				//// returns index of newly added output proc
				//size_t add_output_proc(ConnectableProcessor& proc, size_t from_port_id = PORTID_ALL)
				//{
				//	add_node(proc);
				//	output_procs.push_back(&proc);
				//	return output_procs.size() - 1;

				//}

				//void add_proc(ConnectableProcessor& proc) { add_node(proc); }
				//void add_proc(processor_dag& proc) { add_node(proc); }

				// connect constant value to dynamic processor, don't need to add the edge to dag 
				// i.e. there's no process() method in a const so no processor sequence dependency
				void connect_const(Const &from, ConnectableProcessor& to, size_t from_port = PORTID_DEFAULT, size_t to_port = PORTID_DEFAULT) {
					add_node(to.input_proc());
					from.ConnectTo(to.input_proc(), from_port, to_port);
				}

				void connect_procs(ConnectableProcessor& from, ConnectableProcessor& to, size_t from_port = PORTID_DEFAULT, size_t to_port = PORTID_DEFAULT)
				{
					add_edge(from.output_proc(), to.input_proc());
					from.output_proc().ConnectTo(to.input_proc(), from_port, to_port);
				}				
			};

			/*
			 * Graph of processors
			 *
			 */
			class processor_graph 
			{

				using fiber=compound_processor;
				std::map<semaphore*, fiber*> sem_map;

				using proc_or_const = Connectable<samp_t>;
				using proc = ConnectableProcessor;
				std::map<proc_or_const*, fiber*> proc_map;
				scheduler& s_;
			public:

				processor_graph(scheduler& s = scheduler::get()) : s_(s) {}

				void connect(proc_or_const& from, proc& to)
				{
					// if 'from' is rate-triggering, create a new fiber,  and do new_fiber.connect_procs()  
					// if 'from' is not rate-triggering,  find the fiber containing it.  If found, fiber_containing_from.connect_procs() 
					//		if not found, look for 'to' proc,  and do fiber_containing_to.connect_procs()  
					//			if *still* not found, create a new fiber,  and do new_fiber.connect_procs()  
					auto sem = dynamic_cast<semaphore*>(&from);
					fiber* fib = nullptr;
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
					proc_map[&to] = fib;

					// now fib == a valid fiber.  Do the connection
					const auto const_from = dynamic_cast<Const*>(&from);
					const auto proc_from = dynamic_cast<ConnectableProcessor*>(&from);
					if (const_from)
						fib->connect_const(*const_from, to);
					else 
						fib->connect_procs(*proc_from, to);
				}
			};

		} // proc

	}
}
