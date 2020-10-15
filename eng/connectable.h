#pragma once
#include <eigen3/Eigen/Dense>

#include "eng_traits.h"
#include "dictionary.h"
#include "singleton.h"
#include "factory.h"

namespace sel {



	struct sp_ex : public std::runtime_error
	{
		sp_ex(const char *what) : std::runtime_error(what) {}
	};

	class sp_ex_frozen : public sp_ex { public: sp_ex_frozen() : sp_ex("Cannot add, remove or connect ports once frozen") { } };

	class sp_ex_ports_frozen : public sp_ex { public: sp_ex_ports_frozen() : sp_ex("Cannot add or remove ports once frozen") { } };
	class sp_ex_portwidth_frozen : public sp_ex { public: sp_ex_portwidth_frozen() : sp_ex("Cannot change port width once frozen") { } };
	class sp_ex_port_array_full : public sp_ex { public: sp_ex_port_array_full() : sp_ex("No free port slots") { } };
	class sp_ex_illegal_port : public sp_ex { public: sp_ex_illegal_port() : sp_ex("Illegal port slot") { } };
	class sp_ex_port_name : public sp_ex { public: sp_ex_port_name() : sp_ex("Unknown port name") { } };
	class sp_ex_port_size : public sp_ex { public: sp_ex_port_size() : sp_ex("Illegal port width") { } };
	class sp_ex_no_default_port : public sp_ex { public: sp_ex_no_default_port() : sp_ex("No default port") { } };
	class sp_ex_enable_pin_not_output : public sp_ex { public: sp_ex_enable_pin_not_output() : sp_ex("Enable pin cannot be specified as a destination port when linking output ports") { } };
	class sp_ex_buffer_size_arity : public sp_ex { public: sp_ex_buffer_size_arity() : sp_ex("Input port number differs from buffer size") { } };
	class sp_ex_input_ports_unarrayable : public sp_ex { public: sp_ex_input_ports_unarrayable() : sp_ex("Input ports must all be connected to the same source") { } };
	class sp_ex_illegal_input_port : public sp_ex { public: sp_ex_illegal_input_port() : sp_ex("Illegal input port") { } };
	class sp_ex_illegal_output_port : public sp_ex { public: sp_ex_illegal_output_port() : sp_ex("Illegal output port") { } };
	class sp_ex_nodefault_input_port : public sp_ex { public: sp_ex_nodefault_input_port() : sp_ex("No default input port") { } };
	class sp_ex_nodefault_output_port : public sp_ex { public: sp_ex_nodefault_output_port() : sp_ex("No default output port") { } };
	class sp_ex_input_port_connected : public sp_ex { public: sp_ex_input_port_connected() : sp_ex("Input port already connected") { } };
	class sp_ex_output_port_set : public sp_ex { public: sp_ex_output_port_set() : sp_ex("Output port already exists") { } };
	class sp_ex_input_port_notconnected : public sp_ex { public: sp_ex_input_port_notconnected() : sp_ex("Input port is not connected") { } };
	class sp_ex_pin_arity : public sp_ex { public: sp_ex_pin_arity() : sp_ex("Port width mismatch.") { } };
	class sp_ex_pin_proxy_in_arity : public sp_ex { public: sp_ex_pin_proxy_in_arity() : sp_ex("Input number of ports differ between processor and proxy input processor.  ") { } };
	class sp_ex_pin_proxy_out_arity : public sp_ex { public: sp_ex_pin_proxy_out_arity() : sp_ex("Output number of ports differ between processor and proxy output processor.  ") { } };
	class sp_ex_pin_arity_new_dupin : public sp_ex {
	public: sp_ex_pin_arity_new_dupin() : sp_ex(
		"Port numbers differ."
		"  Make sure the \"from\" processor's input ports are connected before connecting its outputs, "
		"because its number of output ports is dependent on its number of connected input ports.") { }
	};
	class sp_ex_cycle : public sp_ex { public: sp_ex_cycle() : sp_ex("Graph has cycle(s)") { } };
	class sp_ex_no_clock_source : public sp_ex { public: sp_ex_no_clock_source() : sp_ex("Graph has no clock source") { } };

	// well known port names

	struct PortName : public singleton< case_insensitive_dictionary<size_t> >
	{
		static void Add(const char *name, size_t val) { (get())[name] = val; }
		static size_t Get(const char *name) { return (get())[name]; }

	};

	/*
	Class that acts like a basic std::array, but can have up to CAPACITY slots, which can be added after construction
	Slots cannot be removed, but their contents can be changed
	It supports basic forward iteration, including range based "for" loops
	Once the freeze() method is called,  no more slots can be added
	Example:
		slot_array<socket, 12> socks(2,  INVALID_SOCKET );  // create an array of capcity 12 slots, and populate with two uninitialzed sockets
		assert(socks.size() == 2);
		assert(socks.end() == socks.begin() + 2);

		// initialize the two sockets
		for (auto& sock: socks)
			sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		socks[1] == INVALID_SOCKET; // access by index;
		socks[4] == INVALID_SOCKET // throw out_of_range() error
	*/
	template<class T, size_t CAPACITY>class slot_array
	{
	private:
		T _data[CAPACITY];


		size_t num_elems;
		bool _frozen;

		void check_range(size_t idx) const { if (idx >= num_elems) throw std::out_of_range("no such slot"); }
	public:
		bool frozen() const { return _frozen; }
		typedef T * iterator;
		typedef const T * const_iterator;

		const T & operator[](size_t idx) const {
			check_range(idx);
			return _data[idx];
		}

		const T & operator[](const char *idx) const {
			check_range(idx);
			return _data[idx];
		}

		T & operator[](size_t idx) {
			check_range(idx);

			return _data[idx];
		}

		T  *data() { return &_data[0]; }
		const T  *data() const { return &_data[0]; }

		size_t size() const { return num_elems; }

		slot_array(size_t count = 0, const T initial_value = std::is_pointer<T>::value ? nullptr : std::numeric_limits<T>::quiet_NaN()) : num_elems(0), _frozen(false)
		{
			if (count)
				add(initial_value, count);
		}

		const_iterator begin() const { return &_data[0]; }
		iterator begin() { return &_data[0]; }
		const_iterator end() const { return &_data[num_elems]; }
		iterator end() { return &_data[num_elems]; }

		void freeze() { _frozen = true; }

		size_t add(const T p, size_t count = 1)
		{
			if (_frozen)
				throw sp_ex_ports_frozen();
			if (num_elems + count > CAPACITY)
				throw std::out_of_range("no more slots available");

			for (size_t i = 0; i < count; ++i)
				_data[num_elems++] = p;


			return num_elems;
		}


	};

#define PORTS_ARE_EIGEN_ARRAYS
#ifdef PORTS_ARE_EIGEN_ARRAYS

		// Vector of numeric values
	static constexpr size_t port_dynamic_size = Eigen::Dynamic;

	template<size_t W>struct port_t_traits
	{
		static constexpr bool is_dynamic_size() { return W == port_dynamic_size; }
	};

	template<class T, size_t W=port_dynamic_size, bool=port_t_traits<W>::is_dynamic_size()>struct port_t;

	template<class T, size_t W>struct port_t<T, W, true> 
	{
		typedef std::vector<T> vector_t;
	private:
		 vector_t v_;
		mutable bool frozen = false;
	public:
		template<typename>friend class Connectable; // allow Connectable to upcast

		static constexpr auto INVALID_VALUE() { return std::numeric_limits<T>::quiet_NaN(); }

		void freeze() { frozen = true; }
		// width can never be zero
		explicit port_t(size_t width = 1) : v_(width ? width : 1, INVALID_VALUE()) {  }

		template<class IT>port_t(IT first, IT last) : v_(first, last) {}

		// width() is alias for size()
		constexpr size_t width() const { return v_.size(); }

		void setwidth(size_t w) { if (frozen && w != width()) throw sp_ex_portwidth_frozen();  v_.resize(w, INVALID_VALUE()); }
		void freezewidth(size_t w) { if (frozen && w != width()) throw sp_ex_portwidth_frozen();  v_.resize(w, INVALID_VALUE());  freeze(); }

		// as_array is alias for data()
		T *as_array() { return v_.data(); }

		std::vector<T> as_vector() { return v_;  }
		std::vector<T>& as_vector_ref() { return v_;  }

		const T *as_array() const { return v_.data(); }

		auto begin() { return v_.begin(); }
		auto end() { return v_.end(); }
		auto begin() const { return v_.begin(); }
		auto end() const { return v_.end(); }
	};

	template<class T, size_t W>struct port_t<T, W, false> 
	{

		typedef std::array<T, W> vector_t;
	private:
		 vector_t v_;
		const bool frozen = true;
	public:
		template<typename>friend class Connectable; // allow Connectable to upcast

		static constexpr auto INVALID_VALUE() { return std::numeric_limits<T>::quiet_NaN(); }

		void freeze() { }
		// width can never be zero
		explicit port_t() { v_.fill(INVALID_VALUE()); }

		template<class IT>port_t(IT first, IT last) : v_(first, last)
		{
			for (size_t i = 0; i < v_.size(); ++i)
			{
				v_[i] = *first;
				if (++first == last)
					break;
			}

		}

		// width() is alias for size()
		constexpr size_t width() const { return v_.size(); }

		void setwidth(size_t w) { if (w != width()) throw sp_ex_portwidth_frozen();  }
		void freezewidth(size_t w) { if (w != width()) throw sp_ex_portwidth_frozen();  }

		// as_array is alias for data()
		samp_t *as_array() { return v_.data(); }

		std::vector<T> as_vector() { return std::vector<T>(v_.begin(), v_.end());  }
		std::vector<T>& as_vector_ref() { return as_vector();  }

		const samp_t *as_array() const { return v_.data(); }

		auto begin() { return v_.begin(); }
		auto end() { return v_.end(); }
		auto begin() const { return v_.begin(); }
		auto end() const { return v_.end(); }

	};	
#else
		// Vector of numeric values
	static constexpr size_t port_dynamic_size = std::numeric_limits<std::size_t>::max();

	template<size_t W>struct port_t_traits
	{
		static constexpr bool is_dynamic_size() { return W == port_dynamic_size; }
	};

	template<class T, size_t W=port_dynamic_size, bool=port_t_traits<W>::is_dynamic_size()>struct port_t;

	template<class T, size_t W>struct port_t<T, W, true> 
	{

		typedef std::vector<T> vector_t;
	private:
		 vector_t v_;
		mutable bool frozen = false;
	public:
		template<typename>friend class Connectable; // allow Connectable to upcast

		static constexpr auto INVALID_VALUE() { return std::numeric_limits<T>::quiet_NaN(); }

		void freeze() { frozen = true; }
		// width can never be zero
		explicit port_t(size_t width = 1) : v_(width ? width : 1, INVALID_VALUE()) {  }

		template<class IT>port_t(IT first, IT last) : v_(first, last) {}

		// width() is alias for size()
		constexpr size_t width() const { return v_.size(); }

		void setwidth(size_t w) { if (frozen && w != width()) throw sp_ex_portwidth_frozen();  v_.resize(w, INVALID_VALUE()); }
		void freezewidth(size_t w) { if (frozen && w != width()) throw sp_ex_portwidth_frozen();  v_.resize(w, INVALID_VALUE());  freeze(); }

		// as_array is alias for data()
		T *as_array() { return v_.data(); }

		std::vector<T> as_vector() { return v_;  }
		std::vector<T>& as_vector_ref() { return v_;  }

		const T *as_array() const { return v_.data(); }

		auto begin() { return v_.begin(); }
		auto end() { return v_.end(); }
		auto begin() const { return v_.begin(); }
		auto end() const { return v_.end(); }
	};

	template<class T, size_t W>struct port_t<T, W, false> 
	{

		typedef std::array<T, W> vector_t;
	private:
		 vector_t v_;
		const bool frozen = true;
	public:
		template<typename>friend class Connectable; // allow Connectable to upcast

		static constexpr auto INVALID_VALUE() { return std::numeric_limits<T>::quiet_NaN(); }

		void freeze() { }
		// width can never be zero
		explicit port_t() { v_.fill(INVALID_VALUE()); }

		template<class IT>port_t(IT first, IT last) : v_(first, last)
		{
			for (size_t i = 0; i < v_.size(); ++i)
			{
				v_[i] = *first;
				if (++first == last)
					break;
			}

		}

		// width() is alias for size()
		constexpr size_t width() const { return v_.size(); }

		void setwidth(size_t w) { if (w != width()) throw sp_ex_portwidth_frozen();  }
		void freezewidth(size_t w) { if (w != width()) throw sp_ex_portwidth_frozen();  }

		// as_array is alias for data()
		samp_t *as_array() { return v_.data(); }

		std::vector<T> as_vector() { return std::vector<T>(v_.begin(), v_.end());  }
		std::vector<T>& as_vector_ref() { return as_vector();  }

		const samp_t *as_array() const { return v_.data(); }

		auto begin() { return v_.begin(); }
		auto end() { return v_.end(); }
		auto begin() const { return v_.begin(); }
		auto end() const { return v_.end(); }

	};

#endif

	//typedef port_t<samp_t>  port;

	template<class Impl>struct connectable_t
	{
		template<class Impl2>void connect_to(connectable_t<Impl2>& to, size_t from_port, size_t to_port)
		{
			static_cast<Impl*>(this)->ConnectTo(to, from_port, to_port);
		}

		template<class Impl2>void connect_from(connectable_t<Impl2>& from, size_t from_port, size_t to_port)
		{
			static_cast<Impl*>(this)->ConnectFrom(from, from_port, to_port);
		}

	};
	/*
	Subclasses of Connectable will set any required port widths at construction time or even at freeze() time -
	By default,  the ports  are zero width (i.e. port[n].size() == 0 for all output ports)
	but *not* the port widths.  It's up to subclasses to check that the widths of the connected ports are appropriate.
	For example,  if an lpc "expects" its SIGNAL input port to be at least width N (i.e. lpc.inports[0].size() >= N), it must make that check
	in its freeze() method.
	*/

	template<typename T> class Connectable: public connectable_t<Connectable<T>>, public traceable< Connectable<T> >
	{
	private:

		bool frozen;

		static struct PortNames : public singleton< case_insensitive_dictionary<size_t>> {
			PortNames() {
				(get())["MAX"] = PORTID_MAX;
				(get())["DEFAULT"] = PORTID_DEFAULT;
				(get())["NEW"] = PORTID_NEW;
				(get())["ALL"] = PORTID_ALL;
				(get())["ENABLE_PIN"] = PORTID_ENABLE_PIN;
			}
		} portnames;

		static T* enabled() {
			static T enabled = 1.0; return &enabled;
		}


	protected:

		using port = port_t<T>;

		static constexpr size_t MAX_PORTS = 16; // Maximum number of ports (note: the ports' widths are not size-restricted)

		T *enable_pin;

		slot_array<port *, MAX_PORTS> inports;
		slot_array<port *, MAX_PORTS> outports;

		static void set_port_alias(const char *name, size_t val) {
			(portnames.get())[name] = val;
		}


		virtual std::ostream& trace(std::ostream& os) const override
		{
			os << "[ ";
			for (auto p : inports)
				os << p->width() << ' ';
			os << ", ";
			for (auto p : outports)
				os << p->width() << ' ';

			os << "]";
			return os;
		}

	public:

		virtual size_t default_inport() const { return 0; }
		virtual size_t default_outport() const { return 0; }

		virtual void freeze(void)
		{
			if (frozen)
				return;
			for (auto port : inports)
				port->freeze();
			for (auto port : outports)
				port->freeze();

			inports.freeze();
			outports.freeze();
			// check that all inputs are connected -- too late after this function call to connect them
			if (!is_input_connected(PORTID_ALL))
				throw sp_ex_input_port_notconnected();
			frozen = true;
		}
		static constexpr size_t PORTID_0 = 0;
		static constexpr size_t PORTID_1 = 1;
		// reserved port id's
		static constexpr size_t PORTID_UNDEF = std::numeric_limits<std::size_t>::max(); // Use this for initializing to an invalid port id

		static constexpr size_t PORTID_MAX = PORTID_UNDEF - 50;
		static constexpr size_t PORTID_SPECIAL = PORTID_MAX + 1;  // Special ports
		static constexpr size_t PORTID_DEFAULT = PORTID_SPECIAL;
		static constexpr size_t PORTID_NEW = PORTID_SPECIAL + 1;
		static constexpr size_t PORTID_ALL = PORTID_SPECIAL + 2;
		static constexpr size_t PORTID_ENABLE_PIN = PORTID_SPECIAL + 3;
		static constexpr size_t PORTID_NODEFAULT = PORTID_SPECIAL + 4;

		size_t get_port_alias(const char *name) {
			try {
				return portnames.get().at(name);

			}
			catch (std::out_of_range&) {
				throw sp_ex_port_name();
			}
		}


		Connectable() : frozen(false), enable_pin(enabled())
		{
		}

		void assert_valid_inport(const size_t port_id) const {
			if (port_id >= inports.size() && port_id < PORTID_SPECIAL)
				throw sp_ex_illegal_input_port();
		}

		void assert_valid_outport(const size_t port_id) const {
			if (port_id >= outports.size() && port_id < PORTID_SPECIAL)
				throw sp_ex_illegal_output_port();
		}


		bool is_input_connected(const size_t port_id) const
		{
			if (port_id == PORTID_ALL) {
				for (auto port : inports)
					if (nullptr == port)
						return false;
				return true;
			}
			else
				return nullptr != In(port_id);
		}

		bool is_output_connected(const size_t port_id) const
		{
			auto port = Out(port_id);
			return port->width() && !std::isnan(port->as_array()[1]);
		}

		const T *in_as_array(const size_t port_id) const
		{
			return port_id == PORTID_ENABLE_PIN ? enable_pin : inports[port_id]->as_array();
		}

		virtual ~Connectable() = default;


		const port *In(size_t port_id) const
		{
			assert_valid_inport(port_id);

			return inports[port_id];
		}

		port  *Out(size_t port_id) const
		{
			assert_valid_outport(port_id);
			return outports[port_id];
		}

		T *out_as_array(size_t port_id) const
		{
			return port_id == PORTID_ENABLE_PIN ? enable_pin : outports[port_id]->as_array();
		}

		auto& ConnectFrom(const Connectable& from, size_t output_port = PORTID_DEFAULT, size_t input_port = PORTID_DEFAULT)
		{
			auto& to = *this;
			if (to.frozen) throw sp_ex_frozen();

			from.ConnectTo(to, output_port, input_port);
			return to;
		}

		// syntactic sugar
		auto& operator*=(const Connectable& from) { return from.ConnectTo(*this);  }

		const Connectable& ConnectTo(Connectable& to, size_t from_port_id = PORTID_DEFAULT, size_t to_port_id = PORTID_DEFAULT) const
		{
			if (to.frozen) throw sp_ex_frozen();

			if (from_port_id == PORTID_DEFAULT)
				from_port_id = this->default_outport();

			if (to_port_id == PORTID_DEFAULT)
				to_port_id = to.default_inport();

			return ConnectOutputToInput(to, from_port_id, to_port_id);

		}

		const Connectable& ConnectOutputToInput(Connectable& to, size_t from_port_id = PORTID_DEFAULT, size_t to_port_id = PORTID_DEFAULT) const
		{

			if (to.frozen) throw sp_ex_frozen();

			const Connectable& from = *this;

			if (from_port_id == PORTID_DEFAULT)
				from_port_id = from.default_outport();

			if (to_port_id == PORTID_DEFAULT)
				to_port_id = to.default_inport();

			from.assert_valid_outport(from_port_id);
			to.assert_valid_inport(to_port_id);

			size_t op = from_port_id;
			size_t ip = to_port_id;

			if (from_port_id == PORTID_ALL) {
				for (size_t port_id = 0; port_id < from.outports.size(); ++port_id)
					ConnectOutputToInput(to, port_id, 
						to_port_id == PORTID_NEW ? PORTID_NEW : to_port_id == PORTID_ALL ? port_id :  ip++);
			}

			else if (to_port_id == PORTID_NEW) {
				if (to.inports.frozen())
					throw sp_ex_ports_frozen();

				to.inports.add(from.outports[from_port_id]);
			}
			else if (to_port_id == PORTID_ENABLE_PIN)
				to.enable_pin = from.out_as_array(from_port_id);

			else
				if (to.is_input_connected(to_port_id))
					throw sp_ex_input_port_connected();
				else
					to.inports[to_port_id] = from.outports[from_port_id];

			return from;
		}

		const Connectable& ConnectInputToInput(Connectable& to, size_t from_port_id = PORTID_DEFAULT, size_t to_port_id = PORTID_DEFAULT) const
		{

			if (frozen) throw sp_ex_frozen();

			const Connectable&  from = *this;

			if (from_port_id == PORTID_DEFAULT)
				from_port_id = from.default_inport();

			if (to_port_id == PORTID_DEFAULT)
				to_port_id = to.default_inport();


			from.assert_valid_inport(from_port_id);
			to.assert_valid_inport(to_port_id);


			size_t op = from_port_id;
			size_t ip = to_port_id;

			if (from_port_id == PORTID_ALL) {
				for (size_t port_id = 0; port_id < from.inports.size(); ++port_id)
					ConnectInputToInput(to, port_id, to_port_id == PORTID_NEW ? PORTID_NEW : ip++);
			}

			else if (to_port_id == PORTID_NEW) {
				if (to.inports.frozen())
					throw sp_ex_ports_frozen();
				to.inports.add(from.inports[from_port_id]);
			}
			else if (to_port_id == PORTID_ENABLE_PIN)
				to.enable_pin = (T *)from.in_as_array(from_port_id);

			else
				if (to.is_input_connected(to_port_id))
					throw sp_ex_ports_frozen();
				else
					to.inports[to_port_id] = from.inports[from_port_id];

			return from;
		}

		const Connectable& ConnectOutputToOutput(Connectable& to, size_t from_port_id = PORTID_DEFAULT, size_t to_port_id = PORTID_DEFAULT) const
		{

			if (frozen) throw sp_ex_frozen();

			const Connectable&  from = *this;

			if (from_port_id == PORTID_DEFAULT)
				from_port_id = from.default_inport();

			if (to_port_id == PORTID_DEFAULT)
				to_port_id = to.default_inport();

			from.assert_valid_outport(from_port_id);
			to.assert_valid_outport(to_port_id);


			size_t op = from_port_id;
			size_t ip = to_port_id;

			if (from_port_id == PORTID_ALL) {
				for (size_t port_id = 0; port_id < from.outports.size(); ++port_id)
					ConnectOutputToOutput(to, port_id, to_port_id == PORTID_NEW ? PORTID_NEW : ip++);
			}

			else if (to_port_id == PORTID_NEW) {
				if (to.outports.frozen())
					throw sp_ex_ports_frozen();

				to.outports.add(from.outports[from_port_id]);
			}
			else if (to_port_id == PORTID_ENABLE_PIN)
				throw sp_ex_enable_pin_not_output();
			else
				if (to.is_output_connected(to_port_id))
					throw sp_ex_output_port_set();
				else
					to.outports[to_port_id] = from.outports[from_port_id];

			return from;
		}
		// static void Connectable<SIGNAL_T>::Adapt(Connectable<SIGNAL_T> *from, Connectable<COMPLEX_T> *to, PortId output_port, PortId input_port);
	};

	/*template<typename T>class InputChannel {
		using conn = Connectable<T>;

		conn& c;
		size_t p;
	public:
		InputChannel(conn& c, size_t pid = conn::PORTID_DEFAULT) : c(c), p(pid) {}

		InputChannel connect_from(OutputChannel from) {
			from.c.ConnectOutputToInput(from.c, from.p, this->p); 
			return InputChannel(from);
		}

		InputChannel link(InputChannel other) {
			this->c.ConnectInputToInput(other.c, this->p, other.p); 
			return *this;
		}
	};

	template<typename T>class OutputChannel {
		using conn = Connectable<T>;

		const conn& c;
		size_t p;
	public:
		OutputChannel(conn& c, size_t pid = conn::PORTID_DEFAULT) : c(c), p(pid) {}

		OutputChannel connect_to(InputChannel to) {
			this->c.ConnectOutputToInput(to.c, this->p, to.p); 
			return OutputChannel(to.c);
		}

		OutputChannel link(OutputChannel other) {
			this->c.ConnectOutputToOutput(other.c, this->p, other.p);
			return *this;
		}

	};*/

} // sel


