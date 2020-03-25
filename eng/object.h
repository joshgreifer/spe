#pragma once
#include <iostream> // for trace
namespace sel {
	template<class T>class traceable
	{
	public:
		virtual ~traceable() = default;
	private:
		virtual std::ostream& trace(std::ostream& os) const = 0;

		friend std::ostream& operator<<(std::ostream& os, T& ref)
		{
			ref.trace(os);
			return os;
		}
	};
}


