#pragma once
#include "singleton.h"
#include "pugixml.hpp"
#include "msg_and_error.h"
#include "processor.h"

using namespace pugi;
using namespace std;

namespace sel {

	namespace eng {

		class xml_loader : public singleton<xml_loader>
		{
			class xml_loader_ex : public eng_ex {

			public:
				xml_loader_ex(xml_parse_result& parse_result) : eng_ex(parse_result.description()) {}
				xml_loader_ex(const char *description) : eng_ex(description) {}

			};

			charbuf<1000000> doc_;

			// TODO:  Validation
			// bool validateDoc(xml_document& doc);
			const char *preprocess(const char *stream_or_inline_xml, params* args)
			{
				const char *file_name = nullptr;
				bool isURI = false;
				try
				{
					const uri uri(stream_or_inline_xml);
					uri.assert_scheme(uri::FILE);
					stream_or_inline_xml = uri.path();
					isURI = true;
					
				} catch (sel::eng_ex& /* ex */) {}
				
				if (isURI) {
					try {
						doc_.loadFromTextFile(stream_or_inline_xml);
					}
					catch (...) { throw eng_ex(format_message("Couldn't open XML file %s", stream_or_inline_xml)); }
				}
				else
					doc_ = stream_or_inline_xml;

				if (args)
					doc_.macro_expand(*args);

				return doc_;
			}

			const char *attvalue(xml_node& node, const char *attname)
			{
				xml_attribute att = node.attribute(attname);
				if (att != nullptr)
					return  (att.value());
				return "";
			}
			const char *idof(xml_node& node)
			{
				return node.attribute("id").value();
			}

			bool loadProcessorDefinitions(xml_document& doc)
			{
				auto xpath_procdefs = doc.first_child().select_nodes("//compound_proc");

				for (auto xpathnode : xpath_procdefs) {
					auto procdefnode = xpathnode.node();
					params create_params = params(procdefnode);
					create_params["type"] = "compound_proc";
					eng::proc::compound_processor& compound_proc = eng::proc::compound_processor::create_ref(create_params);

					auto xpath_procs = procdefnode.select_nodes("proc");
					for (auto xpathnode : xpath_procs) {
						auto procnode = xpathnode.node();
						auto create_params = params(procnode);
						// prepend compound proc's id to contained processor id (i.e. create namespace)
						charbuf_short cb_id; cb_id.sprintf("%s.%s", idof(procdefnode), create_params["id"]);
						create_params["id"] = cb_id;
						auto *proc = dynamic_cast<eng::ConnectableProcessor *>(abstract_factory::create_from_type(create_params.at("type"), create_params));

						compound_proc.add_node(*proc);
					}
					auto xpath_consts = procdefnode.select_nodes("const");
					for (auto xpathnode : xpath_consts) {
						auto constnode = xpathnode.node();
						auto create_params = params(constnode);
						std::string cb_id = format_message("%s.%s", idof(procdefnode), create_params["id"]);
						auto c = sel::eng::Const::create_ref(create_params);
						instance_manager::set_handle(c, cb_id);
					}

					auto xpath_connections = procdefnode.select_nodes("connect");
					for (auto xpathnode : xpath_connections) {
						auto connectionnode = xpathnode.node();
						charbuf_short cb_from_id; cb_from_id.sprintf("%s.%s", idof(procdefnode), attvalue(connectionnode, "from"));
						charbuf_short cb_to_id; cb_to_id.sprintf("%s.%s", idof(procdefnode), attvalue(connectionnode, "to"));
						auto from_id = cb_from_id.data();
						auto to_id = cb_to_id.data();
						auto obj = &instance_manager::from_handle(from_id);
						auto *from_proc = dynamic_cast<eng::ConnectableProcessor *>(obj);
						auto *from_const = dynamic_cast<eng::Const *>(obj);
						auto *to = dynamic_cast<eng::ConnectableProcessor *>(&instance_manager::from_handle(to_id));
						if (!to)
							throw eng_ex(format_message("Couldn't connect id %s to id %s in %s.  %s is not a connectable processor.", from_id, to_id, idof(procdefnode), to_id));
						if (from_proc) {
							compound_proc.connect_procs(*from_proc, *to);

						} else if (from_const)
							compound_proc.connect_const(*from_const, *to);
						else
							throw eng_ex(format_message("Couldn't connect id %s to id %s in %s.  %s is not a connectable processor or a const.", from_id, to_id, idof(procdefnode), from_id));

					}
				}
				return true;
			}

			bool loadRepeaters(xml_document& doc)
			{
				auto xpath_repeaters = doc.first_child().select_nodes("//repeater");

				for (auto xpathnode : xpath_repeaters) {
					auto repeaternode = xpathnode.node();
					params create_params = params(repeaternode);
					eng::periodic_event& repeater = eng::periodic_event::create_ref(create_params);

				}
				return true;
			}

			bool loadSchedules(xml_document& doc)
			{
				auto xpath_schedules = doc.first_child().select_nodes("//schedule");

				for (auto xpathnode : xpath_schedules) {
					auto schedulenode = xpathnode.node();
					params create_params = params(schedulenode);
					auto trigger_id = create_params.get<std::string>("trigger");
					auto action_id = create_params.get<std::string>("action");

					auto trigger_ = instance_manager::ptr_from_handle<eng::semaphore>(trigger_id);
					if (!trigger_)
						throw eng_ex(format_message("Couldn't add schedule: %s is not a schedule trigger.", trigger_id));

					auto action_ = instance_manager::ptr_from_handle<functor>(action_id);
					if (!action_)
						throw eng_ex(format_message("Couldn't add schedule: %s is not a schedule action (functor).", action_id));

					eng::scheduler::get().add(trigger_, *action_);

				}
				return true;
			}

		public:

			bool load(const char *stream_or_inline_xml, params* args)
			{

				// Initialize objects and variables.
				xml_document doc;
				// Load XML from URL or inline.
				const char *doctext = preprocess(stream_or_inline_xml, args);

				xml_parse_result result = doc.load_string(doctext);
				if (!result)
					throw xml_loader_ex(result);

				if (!loadProcessorDefinitions(doc))
					return false;
				if (!loadRepeaters(doc))
					return false;
				if (!loadSchedules(doc))
					return false;


				try {
					doc.save_file("graph.xml");
				}
				catch (...) {
					std::cerr << "Couldn't save graph to disk.  It will not be possible to merge graphs.\n";
				}

				return true;
			}
		};
	} // eng
} // sel

#if (SUPPORTS_XML_LOADING)
#include <URI.h>
#include <comutil.h>
#include "Eng3.h"

#include "pugixml.hpp"
#include "UserProcFactory.h"

using namespace pugi;
using namespace std;


class Loader : public SINGLETON<Loader>
{
	class xml_loader_ex : public eng_ex {

	public:
		xml_loader_ex(xml_parse_result& parse_result) : eng_ex(parse_result.description()) {}
		xml_loader_ex(const char *description) : eng_ex(description) {}

	};
	// map of merged libraries.
	// when each library is merged, the namespace|location is added here.
	// If a library specifies an already added namespace but different location, the loader throws an error.
	// If a library specifies an already added namespace and identical location, the loader doesnt add the library, 
	//  but doesn't throw any error.  This is similar to C allowing multiple #defines as long as they're identical.


	// https://pugixml.org/docs/manual.html#loading

	std::map<string, string> _merged_libraries;


	std::list<string> _included_files;

	STRINGBUF<1000000> doc_;

	// TODO:  Validation
	// bool validateDoc(xml_document& doc);
	const char *preprocess(const char *stream_or_inline_xml, bool isURI, params* args)
	{
		if (isURI) {
			try {
				doc_.loadFromTextFile(stream_or_inline_xml);
			}
			catch (...) { throw eng_ex(EngineFormatMessage("Couldn't open XML file %s", stream_or_inline_xml)); }
		}
		else
			doc_ = stream_or_inline_xml;

		doc_.macro_expand(*args);

		return doc_;
	}
	const char *attvalue(xml_node& node, const char *attname)
	{
		xml_attribute att = node.attribute(attname);
		if (att != NULL)
			return  (att.value());
		return "";
	}
	const char *idof(xml_node& node)
	{
		return node.attribute("id").value();
	}
	bool loadProcessorDefinitions(xml_document& doc)
	{
		auto xpath_procdefs = doc.first_child().select_nodes("//processor_definition");

		for (auto xpathnode : xpath_procdefs) {
			auto procdef = xpathnode.node();
			params n = params(procdef);
			UserProcFactory *factory = new UserProcFactory(n);

			auto xpath_procs = procdef.select_nodes("processors/*");
			for (auto xpathnode : xpath_procs) {
				auto proc = xpathnode.node();
				factory->AddProcInfo(string(proc.name()), params(proc));
			}

			auto xpath_connections = procdef.select_nodes("internal_connection");
			for (auto xpathnode : xpath_connections) {
				auto connection = xpathnode.node();
				factory->AddConnectionInfo(
					string(attvalue(connection, "from")),
					string(attvalue(connection, "to")),
					string(attvalue(connection, "fromPin")),
					string(attvalue(connection, "toPin")));
			}
		}
		return true;
	}
	bool loadProcessors(xml_document& doc)
	{
		auto xpath_procs = doc.first_child().select_nodes("graph/processors/*");
		for (auto xpathnode : xpath_procs) {
			auto proc = xpathnode.node();
			if (!CreateObject(proc.name(), idof(proc), params(proc)))
				return false;
		}

		return true;
	}
	bool loadBlocks(xml_document& doc)
	{
		auto xpath_blocks = doc.first_child().select_nodes("//block|//schedule");

		for (auto xpathnode : xpath_blocks) {
			auto block = xpathnode.node();
			auto xpath_procs = block.select_nodes("add");
			for (auto xpathnode : xpath_procs) {
				auto proc = xpathnode.node();

				if (!AddProc(idof(block), attvalue(proc, "id")))
					return false;

			}
			auto xpath_connections = block.select_nodes("connection");
			for (auto xpathnode : xpath_connections) {
				auto connection = xpathnode.node();

				if (!Connect(idof(block), attvalue(connection, "from"), attvalue(connection, "to"), params(connection)))
					return false;

			}
			// register the block's triggering processor
			if (!RegisterProc(attvalue(block, "trigger"), "default", idof(block)))
				return false;

		}
		return true;
	}
	bool loadIncludes(xml_document& doc)
	{
		bool bResult = true;

		auto xpath_includes = doc.first_child().select_nodes("//include");

		for (auto xpathnode : xpath_includes) {
			auto include = xpathnode.node();

			xml_document doc2;

			auto location = attvalue(include, "location");

			auto it = find(_included_files.begin(), _included_files.end(), location);

			if (it == _included_files.end())
				_included_files.push_back(location);
			else {
				// already included, just remove node
				include.parent().remove_child(include);
				continue;
			}
			// Try to load the include file into a new doc.
			if (!doc2.load_file(location)) {
				EngineTrace("Failed to load include file '%s'", location);
				return false;
			}
			// Handle nested includes
			if (!loadIncludes(doc2))
				bResult = false;
			else {
				// add include file's children here
				auto include_elems = doc2.first_child().children();
				auto  parent = include.parent();

				if (_stricmp(doc2.first_child().name(), parent.name())) {
					EngineTrace("Failed to load include file '%s' because its contents are "
						"invalid at this point in the main XML document.\n\n"
						"The root node of the included file doesn't match the main document's current node.\n\n"
						"The current node is '%s', while the included file's root node is '%s'.\n",
						location,
						parent.name(),
						doc2.first_child().name()
					);
					bResult = false;

				}
				else {
					for (auto include_elem : include_elems) {
						parent.insert_copy_before(include_elem, include);
					}
					parent.remove_child(include);
				}

			}

		}


		return bResult;
	}
	//xml_parse_result mergeLibraries(xml_document& doc);
	bool LoadXML(const char *stream_or_inline_xml, bool isURI, params* args)
	{
		_merged_libraries.clear();
		_included_files.clear();

		// Initialize objects and variables.
		xml_document doc;
		xml_parse_result result;
		// Load XML from URL or inline.
		const char *doctext = preprocess(stream_or_inline_xml, isURI, args);

		result = doc.load_string(doctext);
		if (!result)
			throw xml_loader_ex(result);

		if (!loadIncludes(doc))
			return false;
		//	if (result = mergeLibraries(doc))
		if (!loadProcessorDefinitions(doc))
			return false;
		if (!loadProcessors(doc))
			return false;

		if (!loadBlocks(doc))
			return false;


		try {
			doc.save_file("graph.xml");
		}
		catch (...) {
			EngineTrace("Couldn't save graph to disk.  It will not be possible to merge graphs.");
		}

		return true;
	}


public:
	bool Loader::LoadFromStream(const char* uri, params *args = 0)
	{
		return LoadXML(uri, true, args);
	}
	bool LoadInline(const char* xml, params *args = 0)
	{
		return LoadXML(xml, false, args);
	}

};



/*
Load include files and merge them into the document.
Note that <include> tags inside include files will  be parsed,
i.e. LoadIncludes() might called recursively.
*/








//namespace Global
//{
//	extern Loader *gLoader;
//}
#endif // #if (SUPPORTS_XML_LOADING)
