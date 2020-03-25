#pragma once
namespace sel {
	template<typename T> struct singleton
	{
		static T &get() {
			static T *instance_;
			if (!instance_)
				instance_ = new T;
			return *instance_;
		}
	};
}