#pragma once
#include <atomic>
#include "dictionary.h"
#include "singleton.h"
#include "msg_and_error.h"
#include <boost/core/demangle.hpp>

//#define TRACE_FACTORY_REGISTRY

namespace sel {


	struct abstract_factory;

	namespace global_ {
		using factorymap = std::map<std::string, abstract_factory *>;

		static singleton<factorymap> factories_;

	}

	struct object
	{
		template<class T>static std::string demangled_cpp_classtype() { return boost::core::demangle(typeid(T).name()); }

		virtual const std::string type() const = 0;
	};

	struct instance_manager
	{
		using instancemap = std::map<std::string, object *>;

		static singleton<instancemap> instances_;
 
		// return generic object or throw invalid id
		static object& from_handle(std::string id)
		{
			try {
				auto instances = instances_.get();

				return *instances.at(id);

			}
			catch (std::out_of_range) {
				throw eng_ex(format_message("'%s' is not an object handle", id.c_str()));
			}
		}
		// returns nullptr if invalid id
		template<class T> static T* ptr_from_handle(std::string id)
		{
			return dynamic_cast<T *>(&from_handle(id));
		}

		// return actual object or throw invalid cast
		template<class T> static T& from_handle(const std::string id)
		{
			auto pobj = &from_handle(id);
			auto pret = dynamic_cast<T *>(pobj);
			if (!pret)
				throw eng_ex(format_message("Can't cast '%s' from %s to %s", id.c_str(), pobj->type(), object::demangled_cpp_classtype<T>().c_str() ));
			return *pret;
		}

		static void set_handle(object& pobj, std::string id)
		{
			auto instances = instances_.get();
			if (instances.find(id) == instances.end())
				instances[id] = &pobj;
			else
				throw eng_ex("Duplicate id");
		}

	};

	struct abstract_factory
	{
		using factorymap = std::map<std::string, abstract_factory *>;

//		static singleton<factorymap> factories_;
		virtual object& create(params& params) const = 0;
		virtual std::string product_type() const = 0;

		static object* create_from_type(std::string type, params& params)
		{
			try {
				auto& factories = global_::factories_.get();

				auto obj_factory = factories.at(type);
				return &obj_factory->create(params);
			}
			catch (std::out_of_range) {
				std::cerr << "'" << type << "' is not a registered object type. Did you forget to create its factory?\n";
				return nullptr;
			}

		}
	};

	template<class T>struct factory : virtual abstract_factory
	{
//		static_assert(std::is_base_of_v<factory_creatable, T>, "Can't create factory for this class, because it is not derived from factory_creatable)");
	private:
		static std::string& register_type()
		{
			static std::string sel_classtype = "";
			static std::atomic_flag typename_is_registered = ATOMIC_FLAG_INIT;
			static singleton<factory> factory_;
			// register factory if not yet registered
			if (!typename_is_registered.test_and_set()) {
				auto& factories = global_::factories_.get();
				factory* registered_factory = &factory_.get();
				T obj;
				sel_classtype = obj.type();
				std::string cplusplus_classtype = object::demangled_cpp_classtype<T>();
				if (factories.find(sel_classtype) == factories.end()) {
					factories[sel_classtype] = registered_factory;
#ifdef TRACE_FACTORY_REGISTRY
					std::cout << "*** " << cplusplus_classtype << " registered as '" << sel_classtype << "'. ***\n";
#endif
				}
				else {
#ifdef TRACE_FACTORY_REGISTRY
					std::cout << format_message("*** %s: class was already registered as '%s'. Registered instead as '%s'. ***\n", cplusplus_classtype.c_str(), factories[sel_classtype]->product_type().c_str(), cplusplus_classtype.c_str());
#endif
					sel_classtype = cplusplus_classtype;
					factories[sel_classtype] = registered_factory;

					// throw eng_ex(msg);
				}

			}

			return sel_classtype;
		}
	public:

		object & create(params& args) const final
		{
			return create_ref(args);
		}

		static T& create_ref(params& args)
		{
			return *(new T(args));
		}

		std::string product_type() const final {
			return register_type();
		}

		
		factory() {
			register_type();
		}


	};


	template<class T>struct creatable : object, factory<T> 
	{

		const std::string type() const override
		{
			std::string type = demangled_cpp_classtype<T>();
			// now munge the class type
			return type;
		}
	};

#if 0
	struct generic_creatable {

		typedef const char *handle_t;
		typedef case_insensitive_dictionary<generic_creatable *> object_map_t;

		virtual generic_creatable* create(params& params, handle_t id = nullptr) const = 0;
		virtual std::string type_name() const = 0;

		typedef singleton<object_map_t> object_instances_singleton;

		handle_t id;

		generic_creatable() : id("Unnamed Object") {}

		generic_creatable(params& params) {}

		template<class T> static T *cast_instance_or_throw(handle_t id)
		{
			auto obj = cast_instance<T>(id);
			if (!obj)
				throw eng_ex(format_message("Couldn't cast object id %s to %s.", id, typeid(T).name()));
			return obj;
		}

		template<class T> static T *cast_instance(handle_t id)
		{
			return dynamic_cast<T *>(get_instance(id));
		}

		static generic_creatable *get_instance(handle_t id) {

			object_map_t& object_map = object_instances_singleton::get();

			try {
				return object_map.at(id);
			}
			catch (std::out_of_range&) {
				throw eng_ex(format_message<2048U>("Object '%s' does not exist", id));
			}
		}

		static generic_creatable* create_instance_of(const std::string& type_name_or_alias, params& params, handle_t id = nullptr)
		{
			object_map_t& object_map = object_instances_singleton::get();
			try {
				generic_creatable *factory = object_map.at(type_name_or_alias);

				try {
					return factory->create(params, id);

				}
				catch (std::exception& ex) {
					throw eng_ex(format_message<2048U>("Could not create instance of '%s': %s", type_name_or_alias.c_str(), ex.what()));

				}
			}
			catch (std::out_of_range&) {
				throw eng_ex(format_message<2048U>("Alias '%s' is not registered", type_name_or_alias.c_str()));
			}
		}


		virtual ~generic_creatable() {}
	};

	template<class T>struct creatable : public generic_creatable, virtual public traceable<T>
	{

		virtual std::ostream& trace(std::ostream& os) const override
		{
			os << this->type();
			return os;
		}

//		virtual const std::string type() const override { return traceable<T>::human_readable_typename(); };

		creatable(const char *alias = nullptr) {
			static std::atomic_flag typename_is_registered = ATOMIC_FLAG_INIT;
			if (!typename_is_registered.test_and_set()) {
//				 auto derived = dynamic_cast<traceable<T> *>(this);
				register_factory(this->type());
			}


		}

		static bool register_factory(const std::string& type_name_or_alias)
		{

			object_map_t& object_map = object_instances_singleton::get();
			for (auto& i : object_map)
				if (dynamic_cast<T *>(i.second)) {
					object_map[type_name_or_alias] = i.second;  // factory already added, re-use that factory
					return true;
				}

			if (object_map.find(type_name_or_alias) != object_map.end())
				throw eng_ex(format_message<2048U>("Alias '%s' already registered to class '%s'", type_name_or_alias, object_map[type_name_or_alias]->type_name()));
			object_map[type_name_or_alias] = new T;
#ifdef TRACE_FACTORY_REGISTRY
			std::cout << "*** Registered " << traceable<T>::human_readable_typename() << " as '" << type_name_or_alias << "' ***\n";
#endif
			return true;
		}

		static T &create_instance(params& params, handle_t id = nullptr)
		{
			T foo; // force registration
			object_map_t& object_map = object_instances_singleton::get();
			T *obj = dynamic_cast<T *>(generic_creatable::create_instance_of(traceable<T>::human_readable_typename(), params, id));
			if (!obj)
				throw eng_ex(format_message<2048U>("Cannot cast '%s' from '%s' to '%s'", id, object_map[id]->type_name(), traceable<T>::human_readable_typename().c_str()));
			return *obj;
		}

		static T &get_instance(handle_t id)
		{
			object_map_t& object_map = object_instances_singleton::get();
			T *obj = dynamic_cast<T *>(generic_creatable::get_instance(id));
			if (!obj)
				throw eng_ex(format_message<2048U>("Cannot cast '%s' from '%s' to '%s'", id, object_map[id]->type_name(), traceable<T>::human_readable_typename().c_str()));
			return *obj;
		}


		std::string type_name() const final {
			return this->type(); //  human_readable_typename();
		}

		generic_creatable *create(params& params, handle_t id = nullptr) const final
		{
			object_map_t& object_map = object_instances_singleton::get();

			// if no id passed, use the "ID" param, or throw an exception if none provided
			if (!id)
				id = params.at("id");

			else // add the id to the passed in params, so caller can find out the id of the newly created object
				params["id"] = id;

			T* obj = new T(params);
			obj->id = id;
			if (object_map.find(id) != object_map.end())
				throw eng_ex(format_message<2048U>("object id '%s' already exists as an instance of '%s'", id, object_map[id]->type_name()));
			object_map[id] = obj;
			return obj;
		}


	};
#endif // 0
}