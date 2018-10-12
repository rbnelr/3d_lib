#pragma once

#include "engine_include.hpp"
#include "engine_dear_imgui.hpp"
#include "engine_options_graph.hpp"

// Convenient way to have variables that are tweakable per gui and saveable to disk 

namespace engine {
namespace options {

class Options {
public:
	
	cstr			save_filepath;

	Options_Graph	graph;

	bool trigger_load;

	Options (cstr save_filepath): save_filepath{save_filepath} {}
private:
	
	template <typename T> bool _value (Node* n, T* val, unit_e unit) {
		if (trigger_load && n->matches(val)) {
			n->get(val);
		}
		
		bool changed = false;//imgui(n->name, val, unit) || trigger_load;
		
		n->assign(*val, Value::get_type(val), unit);
		
		return changed;
	}
	template <typename T> T _value (Node* n, T default_val, unit_e unit) {
		T val = default_val;
		
		if (n->matches(&val)) {
			n->get(&val);
		}
		
		//imgui(n->name, &val, unit);
		
		n->assign(val, Value::get_type(&val), unit);
		
		return val;
	}

	template <typename T> bool _value (cstr name, T* val, unit_e unit) {					return _value(graph.get_node(name			),	val, unit); }
	template <typename T> bool _value (cstr name, int arr_indx, T* val, unit_e unit) {		return _value(graph.get_node(name, arr_indx	),	val, unit); }
	template <typename T> T _value (cstr name, T default_val, unit_e unit) {				return _value(graph.get_node(name			),	default_val, unit); }
	template <typename T> T _value (cstr name, int arr_indx, T default_val, unit_e unit) {	return _value(graph.get_node(name, arr_indx	),	default_val, unit); }

public:
	////
	template <typename T> bool value (cstr name, T* val) {						return _value(name,				val, DIMENSIONLESS); }
	template <typename T> bool value (cstr name, int arr_indx, T* val) {		return _value(name, arr_indx,	val, DIMENSIONLESS); }
	// same as value(), just specifies that values is an angle (can be a vector of angles)
	template <typename T> bool angle (cstr name, T* val) {						return _value(name,				val, ANGLE); }
	template <typename T> bool angle (cstr name, int arr_indx, T* val) {		return _value(name, arr_indx,	val, ANGLE); }

	template <typename T> T value (cstr name, T default_val) {					return _value(name,				default_val, DIMENSIONLESS); }
	template <typename T> T value (cstr name, int arr_indx, T default_val) {	return _value(name, arr_indx,	default_val, DIMENSIONLESS); }
	template <typename T> T angle (cstr name, T default_val) {					return _value(name,				default_val, ANGLE); }
	template <typename T> T angle (cstr name, int arr_indx, T default_val) {	return _value(name, arr_indx,	default_val, ANGLE); }
	
	// like value(), but creates a value-less node and makes it the current parent (following value() nodes will be children of this node)
	//  must always close a begin() with and end()
	void begin (cstr name) {
		graph.begin(name);
	}
	void end () {
		graph.end();
	}
	
	// like begin() / end(), but children will be array members, which means that they all should have the same name and must also be one node per array element (begin() / end())
	int begin_array (cstr name) {
		return graph.begin_array(name);
	}
	void end_array () {
		graph.end_array();
	}

	//
	void begin_frame (bool trigger_load) {
		this->trigger_load = trigger_load;

		if (trigger_load) {
			if (!from_xml(&graph, save_filepath)) {
				errprint("Options: Could load from xml!\n");
			}
		}

		graph.begin_frame();
	}
	void end_frame (bool trigger_save) {
		graph.end_frame();

		if (trigger_save)
			if (!to_xml(graph, save_filepath))
				errprint("Options: Could not save to xml!\n");
	}

};

}
using options::Options;
}
