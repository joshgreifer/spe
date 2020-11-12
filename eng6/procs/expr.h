#pragma once

// processor that evaluates arbitrary expression at runtime
// See http://partow.net/programming/exprtk/




#undef min
#undef max
#undef free // conflicts with MSVC debug free
#include "../params.h"
#include "../eng_traits.h"
#include "../processor.h"
namespace sel {
	namespace eng6 {
		namespace proc {
			class expr : public  ProcessorXxYy, virtual public creatable<expr>
			{
				typedef exprtk::symbol_table<samp_t> symbol_table_t;
				typedef exprtk::expression<samp_t>     expression_t;
				typedef exprtk::parser<samp_t>             parser_t;
				typedef exprtk::parser_error::type			error_t;

				symbol_table_t symbol_table;
				expression_t   init_expression;
				expression_t   process_expression;
				parser_t       parser;

				params varmap;

				std::string process_expression_str;
				std::string init_expression_str;
				std::map<const char *, samp_t>state_variables;
				std::map<const char *, size_t>input_names;
				std::map<const char *, size_t>output_names;

				port retval;

				void compile_expr(symbol_table_t& symtab, std::string& strexpr, expression_t& expr)
				{
					expr.register_symbol_table(symtab);

					if (!parser.compile(strexpr, expr)) {
						charbuf_long all_err_msgs;
						charbuf_short err_msg;
						size_t nerrs = parser.error_count();
						if (nerrs > 1) {
							all_err_msgs.sprintf("%zd errors in expr '%s':\n", nerrs, strexpr.c_str());
							for (std::size_t i = 0; i < nerrs; ++i) {

								error_t error = parser.get_error(i);

								err_msg.sprintf("%s error at pos %zd: %s \n",
									exprtk::parser_error::to_str(error.mode).c_str(),
									error.token.position + 1,
									error.diagnostic.c_str());
								all_err_msgs.append(err_msg);

							}

						}
						else {
							error_t error = parser.get_error(0);
							all_err_msgs.sprintf("%s error in expr '%s' at pos %zd: %s\n",
								exprtk::parser_error::to_str(error.mode).c_str(),
								strexpr.c_str(),
								error.token.position + 1,
								error.diagnostic.c_str());

						}

						throw eng_ex(all_err_msgs);
					}
				}
			public:
				virtual const std::string type() const override { return "expression"; }

				// default constuctor needed for factory creation
				expr() : process_expression_str(""), init_expression_str("") {}

				expr(params& args) :
					varmap(args),
					process_expression_str(args.get<std::string>("expr")),
					init_expression_str(args.get<std::string>("initexpr", ""))
				{
					// port 0 always available
					this->outports.add(&retval);

					// Assign any variables from create args which start with "[scio]_" )
					for (auto& arg : varmap) {
						const char *varname_with_prefix = arg.first.c_str();
						if (varname_with_prefix[1] == '_') {
							char prefix = varname_with_prefix[0];
							const char *varname = varname_with_prefix + 2; // skip prefix
							samp_t val;
							size_t port_id;
							switch (prefix) {
							case 's':
								val = params::convert<samp_t>(arg.second);
								state_variables[varname] = val;
								symbol_table.add_variable(varname, state_variables[varname]);
								break;
							case 'i':
								port_id = params::convert<size_t>(arg.second);
								if (port_id >= inports.size())
									inports.add(nullptr, port_id - inports.size() + 1);
								input_names[varname] = port_id;
								break;
							case 'o': // secondary outputs (there will always be at least  one output)
								port_id = params::convert<size_t>(arg.second);
								if (port_id >= outports.size())
									outports.add(nullptr, port_id - outports.size() + 1);
								output_names[varname] = port_id;
								break;
							case 'c':
								val = params::convert<samp_t>(arg.second);
								state_variables[varname] = val;
								symbol_table.add_variable(varname, state_variables[varname], true);
								break;
							default:
								throw eng_ex(format_message<2048>("Invalid expression variable prefix - %s", varname_with_prefix));

							}
						}

					}
				}

				virtual ~expr()
				{
				}

				void process() final
				{
					samp_t retval1 = process_expression.value();

				}

				void freeze(void) final 
				{
					retval.setwidth(this->inports[0]->width());
					symbol_table.add_vector("output", retval.as_vector_ref());

					// bind input names (has to be done after input is connected)
					for (const auto kv : input_names)
						symbol_table.add_vector(kv.first, this->inports[kv.second]->as_vector_ref());
					
					for (const auto kv : output_names) 
						symbol_table.add_vector(kv.first, this->outports[kv.second]->as_vector_ref());
					
					char varname[256];

					for (size_t i = 0; i < inports.size(); ++i) {
						auto& port = *this->inports[i];

						snprintf(varname, 256, "input%zd", i + 1);
						symbol_table.add_vector(varname, port.as_vector_ref());
					}
					
					for (size_t i = 0; i < outports.size(); ++i) {
						auto& port = *this->outports[i];
						
						snprintf(varname, 256, "output%zd", i + 1);
						symbol_table.add_vector(varname, port.as_vector_ref());
					}
					compile_expr(symbol_table, process_expression_str, process_expression);

					if (init_expression_str != "") {
						compile_expr(symbol_table, init_expression_str, init_expression);
						auto unused_value = init_expression.value();
					}
					Connectable<samp_t>::freeze();
				}
				void init(schedule *context) final
				{

				}
			};
		}
	}
}


#if defined(COMPILE_UNIT_TESTS)
#include "../unit_test.h"

using namespace sel::eng6::proc;

SEL_UNIT_TEST(expr)

		sel::params create_params = {{
					"id", "log_ratio", 
					"expr", "output := 5 * A + B", 
					"i_A", "0", 
					"i_B", "1"}};


void run()
{
	expr expr1(create_params);
	
		const sel::eng6::Const A =  std::vector<double>{{ 7.0, 10.0, 3.0 }} ;
		const sel::eng6::Const B =  std::vector<double>{{ 6.0, -2.0, 1.75 }};

		A.ConnectTo(expr1, 0, 0);
		B.ConnectTo(expr1, 0, 1);

		expr1.freeze();
		expr1.process();
		const auto& results = expr1.Out(0)->as_vector();
		for (size_t i = 0; i < results.size(); ++i)
			SEL_UNIT_TEST_ASSERT(5 * A.at(i) + B.at(i) == results[i])
}
SEL_UNIT_TEST_END
#endif