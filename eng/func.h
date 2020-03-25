#pragma once
#include <functional>
#include <string>
namespace sel {

	typedef std::function<void()> func;

	struct functor : public traceable<functor>
	{
		virtual std::string function_name() const  { return "functor"; }

		virtual std::ostream& trace(std::ostream& os) const override
		{
			os << this->function_name();
			return os;
		}

		virtual void operator()() = 0;
	};

	class function_object : public functor, public traceable<function_object>
	{
		const char *name_;
		func op_;
	public:

		virtual std::ostream& trace(std::ostream& os) const override
		{
			os << function_name();
			return os;
		}

		virtual std::string function_name() const override { return name_; }

		function_object(const char *name = "unnamed") : name_(name) {}
		function_object(func f, const char *name = "unnamed std::function") : name_(name), op_(f) {}
		void operator()() final { op_(); }
	};
} // sel