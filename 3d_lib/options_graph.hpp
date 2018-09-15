#pragma once

#include "engine_include.hpp"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

namespace engine {
namespace options {
//

enum type_e : u16 {
	NULL_=0, // special value
	
	STRUCT, // node contains other nodes (whose names must be unique in this collection)
	ARRAY, // node contains array of nodes of same name

	// 
	STRING,

	INT_, IV2, IV3, IV4,
	FLT, FV2, FV3, FV4,
	BOOL, BV2, BV3, BV4,

	//SRGB8, SRGBA8,
	//LRF, LRGF, LRGBF, LRGBAF,
	
	TYPE_E_COUNT
};
enum unit_e : u16 {
	DIMENSIONLESS=0,
	ANGLE, // in this struct angles are stores as radiants, but are converted on write out to file and read in
};

static cstr typename_ (type_e t) {
	switch (t) {
		case NULL_	:	return "null";

		case STRUCT	:	return "struct";
		case ARRAY	:	return "array";

		case STRING	:	return "string";

		case INT_	:	return "int";
		case IV2	:	return "iv2";
		case IV3	:	return "iv3";
		case IV4	:	return "iv4";

		case FLT	:	return "flt";
		case FV2	:	return "v2";
		case FV3	:	return "v3";
		case FV4	:	return "v4";

		case BOOL	:	return "bool";
		case BV2	:	return "bv3";
		case BV3	:	return "bv4";
		case BV4	:	return "bv4";

			//SRGB8, SRGBA8,
			//LRF, LRGF, LRGBF, LRGBAF,

		default: assert(not_implemented); return "-- error --";
	}
}

static bool union_str_active (type_e t) { return t == STRING; } // which types use str member of union

class Value {
	type_e	type;
	unit_e	mode;

	union {
		std::string		str		; // string is in an invalid state after default construction of Value class

		int				int_	;
		iv2				iv2_	;
		iv3				iv3_	;
		iv4				iv4_	;

		flt				flt_	;
		v2				v2_		;
		v3				v3_		;
		v4				v4_		;

		bool			bool_	;
		bv2				bv2_	;
		bv3				bv3_	;
		bv4				bv4_	;

		//srgb8			srgb8_;
		//srgba8			srgba8_;
		// LRF - LRGBAF just map to flt_ - v4_ here

	};

public:
	Value () {}
	~Value () {
		using std::string;
		if (union_str_active(type))
			str.~string(); // placement destruct string member of union

	}

	type_e get_type () const { return type; }

	bool is_array () const { return type == ARRAY; }
	bool is_container () const { return type == STRUCT || type == ARRAY; }

	void assign (void const* val, type_e new_type, unit_e new_mode=Value::DIMENSIONLESS) {
		if (union_str_active(type) != union_str_active(new_type)) { // either was string and is not anymore or the reverse
			using std::string;

			if (union_str_active(new_type))
				new (&str)string(); // placement construct string member of union
			else // union_str_active(type)
				str.~string(); // placement destruct string member of union

		}

		type = new_type;
		mode = new_mode;

		_assign(val);
	}

	// get type at compile-time
	static type_e get_type (std::string* val) { return STRING	; }
	
	static type_e get_type (int		   * val) { return INT_		; }
	static type_e get_type (iv2		   * val) { return IV2		; }
	static type_e get_type (iv3		   * val) { return IV3		; }
	static type_e get_type (iv4		   * val) { return IV4		; }
	
	static type_e get_type (flt		   * val) { return FLT		; }
	static type_e get_type (v2		   * val) { return FV2		; }
	static type_e get_type (v3		   * val) { return FV3		; }
	static type_e get_type (v4		   * val) { return FV4		; }
	
	static type_e get_type (bool	   * val) { return BOOL		; }
	static type_e get_type (bv2		   * val) { return BV2		; }
	static type_e get_type (bv3		   * val) { return BV3		; }
	static type_e get_type (bv4		   * val) { return BV4		; }

	// assign value
	void _assign (void const* val) {
		switch (type) {
			case STRUCT:
			case ARRAY: return; // no value to assign
			
			case STRING	:
			/*case LOADED	:*/	str		= *(std::string	*)val;	return;

			case INT_	:	int_	= *(int			*)val;	return;
			case IV2	:	iv2_	= *(iv2			*)val;	return;
			case IV3	:	iv3_	= *(iv3			*)val;	return;
			case IV4	:	iv4_	= *(iv4			*)val;	return;

			case FLT	:	flt_	= *(flt			*)val;	return;
			case FV2	:	v2_		= *(v2			*)val;	return;
			case FV3	:	v3_		= *(v3			*)val;	return;
			case FV4	:	v4_		= *(v4			*)val;	return;

			case BOOL	:	bool_	= *(bool		*)val;	return;
			case BV2	:	bv2_	= *(bv2			*)val;	return;
			case BV3	:	bv3_	= *(bv3			*)val;	return;
			case BV4	:	bv4_	= *(bv4			*)val;	return;

			//SRGB8, SRGBA8,
			//LRF, LRGF, LRGBF, LRGBAF,

			default: assert(not_implemented); return;
		}
	}
	void get (void* val) {
		switch (type) {
			case STRING	:	*(std::string	*)val = str		;	return;
			
			case INT_	:	*(int			*)val = int_	;	return;
			case IV2	:	*(iv2			*)val = iv2_	;	return;
			case IV3	:	*(iv3			*)val = iv3_	;	return;
			case IV4	:	*(iv4			*)val = iv4_	;	return;
			
			case FLT	:	*(flt			*)val = flt_	;	return;
			case FV2	:	*(v2			*)val = v2_		;	return;
			case FV3	:	*(v3			*)val = v3_		;	return;
			case FV4	:	*(v4			*)val = v4_		;	return;
			
			case BOOL	:	*(bool			*)val = bool_	;	return;
			case BV2	:	*(bv2			*)val = bv2_	;	return;
			case BV3	:	*(bv3			*)val = bv3_	;	return;
			case BV4	:	*(bv4			*)val = bv4_	;	return;

			//SRGB8, SRGBA8,
			//LRF, LRGF, LRGBF, LRGBAF,

			default: assert(not_implemented); return;
		}
	}

	static bool _parse (std::string const& s, std::string* out) {
		*out = s;
		return true;
	}

	template <typename T, typename F>
	static bool _parse_vec (std::string const& s, T* out, int comp_count, F parse_scalar) {
		T buf[4];
		assert(comp_count <= ARRLEN(buf));

		using namespace n_parse;
		char* cur = (char*)s.c_str();

		int i;
		for (i=0; i<comp_count;) {
			whitespace(&cur);

			if (!parse_scalar(&cur, &buf[i]))
				return false;

			whitespace(&cur);

			++i;
			if (!character(&cur, ','))
				break;
		}

		int comp_found = i;

		if (!(comp_found == 1 || comp_found == comp_count))
			return false;

		if (!end_of_input(cur))
			return false;

		for (int i=0; i<comp_count; ++i) {
			out[i] = buf[ comp_found == 1 ? 0 : i ];
		}
		return true;
	}
	static bool _parse (std::string const& s, int* out, int comp_count, unit_e mode) {
		return _parse_vec(s, out, comp_count, n_parse::signed_int);
	}
	static bool _parse (std::string const& s, flt* out, int comp_count, unit_e mode) {
		return _parse_vec(s, out, comp_count, [&] (char** pcur, flt* out) -> bool {
				using namespace n_parse;
				char* cur = *pcur;

				flt f;

				if (!float32(&cur, &f))
					return false;

				whitespace(&cur);

				if (mode == ANGLE) {
					if (identifier_ignore_case(&cur, "rad")) {
						// alread in radiants
					} else {
						identifier_ignore_case(&cur, "deg"); // "deg" is optional

						f = to_rad(f);
					}
				}

				*out = f;
				*pcur = cur;
				return true;
			});
	}
	static bool _parse (std::string const& s, bool* out, int comp_count, unit_e mode) {
		return _parse_vec(s, out, comp_count, n_parse::bool_);
	}

	static bool _parse (type_e type, unit_e unit, std::string const& s, void* val) {
		switch (type) {
			case STRING	:	return _parse(s, (std::string*)val);

			case INT_	:	return _parse(s,   (int	*)val	 , 1, unit);
			case IV2	:	return _parse(s, &((iv2	*)val)->x, 2, unit);
			case IV3	:	return _parse(s, &((iv3	*)val)->x, 3, unit);
			case IV4	:	return _parse(s, &((iv4	*)val)->x, 4, unit);

			case FLT	:	return _parse(s,   (flt	*)val	 , 1, unit);
			case FV2	:	return _parse(s, &((v2	*)val)->x, 2, unit);
			case FV3	:	return _parse(s, &((v3	*)val)->x, 3, unit);
			case FV4	:	return _parse(s, &((v4	*)val)->x, 4, unit);
			
			case BOOL	:	return _parse(s,   (bool*)val	 , 1, unit);
			case BV2	:	return _parse(s, &((bv2	*)val)->x, 2, unit);
			case BV3	:	return _parse(s, &((bv3	*)val)->x, 3, unit);
			case BV4	:	return _parse(s, &((bv4	*)val)->x, 4, unit);

				//SRGB8, SRGBA8,
				//LRF, LRGF, LRGBF, LRGBAF,

			default: assert(not_implemented); return false;
		}
	}

	static bool parse (type_e type, unit_e unit, std::string const& s, void* val) {
		if (type == STRUCT || type == ARRAY) {
			if (s.size() != 0) {
				fprintf(stderr, "Stray value \"%s\" in %s node!\n", s.c_str(), typename_(type));
				return false;
			}
			return true; // no value to load
		} else {
			
			if (!_parse(type, unit, s, val)) {
				fprintf(stderr, "Could not parse \"%s\" into %s!\n", s.c_str(), typename_(type));
				return false;
			}
			return true;
		}
	}

	// print value into text format
	std::string print (int const* v, int count) const {
		std::string str;
		for (int i=0; i<count; ++i)
			prints(&str, "%d%s", v[i], i == (count -1) ? "":", ");
		return str;
	}
	std::string print (flt const* v, int count) const {
		std::string str;
		for (int i=0; i<count; ++i)
			if (mode == ANGLE)
				prints(&str, "%g deg%s", to_deg(v[i]), i == (count -1) ? "":", ");
			else
				prints(&str, "%g%s", v[i], i == (count -1) ? "":", ");
		return str;
	}
	std::string print (bool const* v, int count) const {
		std::string str;
		for (int i=0; i<count; ++i)
			prints(&str, "%s%s", v[i] ? "true":"false", i == (count -1) ? "":", ");
		return str;
	}

	std::string print () const {
		switch (type) {
			case STRING	:
			/*case LOADED	:*/	return str;
			
			case INT_	:	return print( &int_		, 1	);
			case IV2	:	return print( &iv2_.x	, 2	);
			case IV3	:	return print( &iv3_.x	, 3	);
			case IV4	:	return print( &iv4_.x	, 4	);
			
			case FLT	:	return print( &flt_		, 1	);
			case FV2	:	return print( &v2_.x	, 2	);
			case FV3	:	return print( &v3_.x	, 3	);
			case FV4	:	return print( &v4_.x	, 4	);
			
			case BOOL	:	return print( &bool_	, 1	);
			case BV2	:	return print( &bv2_.x	, 2	);
			case BV3	:	return print( &bv3_.x	, 3	);
			case BV4	:	return print( &bv4_.x	, 4	);

				//SRGB8, SRGBA8,
				//LRF, LRGF, LRGBF, LRGBAF,

			default: assert(not_implemented); return "--error--";
		}
	}
	
};

struct Node {
	std::string			name;

	Value				val;

	Node*				parent = nullptr;
	
	std::vector< unique_ptr<Node> >	children;

	//void update (void* val_, bool load, type_e val_type, unit_e val_mode) { // name and parent set externally
	//	if (load) {
	//		val.get(val_, val_type, val_mode);
	//	}
	//	
	//	val.assign(val_, val_type, val_mode);
	//	
	//	if (!val.is_container()) {
	//		children.clear(); // so we don't end up with both children and a value
	//	}
	//}

	template <typename T> bool matches (T* rval) {
		return val.get_type() == Value::get_type(rval);
	}

	template <typename T> void get (T* pval) {
		assert(matches(pval));

		val.get(pval);
	}
	template <typename T> void assign (T const& pval, type_e type, unit_e unit) {
		val.assign(&pval, type, unit);
	}
};

struct Options_Graph {

	// This represents the root node
	std::vector< unique_ptr<Node> >	root_children;

	Node*				cur_parent = nullptr;

	void begin_frame () {
		
	}
	void end_frame () {
		if (cur_parent)
			fprintf(stderr, "Options: too few end() calls!\n");
		cur_parent = nullptr;
	}

	std::vector< unique_ptr<Node> >* get_children (Node* parent) {
		return parent ? &parent->children : &root_children;
	}

	Node* node_insert (Node* parent, int insert_indx, cstr name) {
		auto* parent_children = get_children(parent);
		
		auto p = make_unique<Node>();
		auto* n = p.get();
	
		n->name = name;
		n->parent = parent;
		parent_children->emplace(parent_children->begin() +insert_indx, std::move(p));
		
		n->val.assign(nullptr, STRUCT, DIMENSIONLESS);

		return n;
	}
	
	Node* find_node (cstr name) {
		auto* cur_children = get_children(cur_parent);
	
		auto res = std::find_if(cur_children->begin(), cur_children->end(), [&] (unique_ptr<Node> const& n) { return n->name.compare(name) == 0; });
		if (res == cur_children->end())
			return nullptr; // node not found
	
		return res->get();
	}
	Node* find_array_node (cstr name, int indx) {
		auto* cur_children = get_children(cur_parent);
	
		if (!(indx >= 0 && indx < (int)cur_children->size()))
			return nullptr; // indx out of range
	
		auto* n = (*cur_children)[indx].get();
	
		if (n->name.compare(name) != 0)
			return nullptr;
		
		return n;
	}
	
	std::vector<int> cur_array_indx;
	
	Node* get_array_node (cstr name, int indx) {
		assert(cur_parent && cur_parent->val.is_array());
	
		auto* n = find_array_node(name, indx);
		if (!n) {
			n = node_insert(cur_parent, indx, name);
		}
		return n;
	}
	Node* get_normal_node (cstr name) { // find existing node or insert new (which is then uninitialized)
		auto* n = find_node(name);
		if (!n) {
			n = node_insert(cur_parent, (int)get_children(cur_parent)->size(), name);
		}
		return n;
	}
	
	Node* get_node (cstr name) {
		if (cur_parent && cur_parent->val.is_array()) {
			return get_array_node(name, cur_array_indx.back()++);
		} else {
			return get_normal_node(name);
		}
	}
	Node* get_node (cstr name, int indx) {
		return get_array_node(name, indx);
	}
	
	//Node* _node (cstr name, void* val, type_e val_type, Value::mode_e val_mode) {
	//	auto* n = get_node(name);
	//	n->update(val, trigger_load, val_type, val_mode);
	//	return n;
	//}
	//Node* _array_node (cstr name, int indx, void* val, type_e val_type, Value::mode_e val_mode) {
	//	auto* n = get_array_node(name, indx);
	//	n->update(val, trigger_load, val_type, val_mode);
	//	return n;
	//}
	//
	//// finds/inserts and updates node at under current parent node (either root or nodes created by open begin() call)
	////  names are searched, so they must be unique (or else the prev node with that names will be overwritten)
	//template <typename T> inline void value (cstr name, T* val) {
	//	_node(name, val, Value::get_type(val), Value::DIMENSIONLESS);
	//}
	//template <typename T> inline void angle (cstr name, T* val) { // same as value(), just specifies that values is an angle (can be a vector of angles)
	//	_node(name, val, Value::get_type(val), Value::ANGLE);
	//}
	//template <typename T> inline void value (cstr name, T* val, int arr_indx) {
	//	_array_node(name, arr_indx, val, Value::get_type(val), Value::DIMENSIONLESS);
	//}
	//template <typename T> inline void angle (cstr name, T* val, int arr_indx) { // same as value(), just specifies that values is an angle (can be a vector of angles)
	//	_array_node(name, arr_indx, val, Value::get_type(val), Value::ANGLE);
	//}
	//
	// like value(), but creates a value-less node and makes it the current parent (following value() nodes will be children of this node)
	//  must always close a begin() with and end()
	void begin (cstr name) {
		auto* n = get_node(name);
		cur_parent = n;
	}
	void end () {
		if (!cur_parent)
			fprintf(stderr, "Save: too many end() calls!\n");
		else
			cur_parent = cur_parent->parent;
	}
	
	// like begin() / end(), but children will be array members, which means that they all should have the same name and must also be one node per array element (begin() / end())
	//  on load: returns the number of subelements (so you can resize your array to that size and then load the values by calling begin() that many times) 
	//  else:	 return -1, increase size of array by calling begin() n times, if you call begin() less times than there are elements the remaining ones will be kept for now (could be removed in )
	int begin_array (cstr name) {
		auto* n = get_node(name);
		n->val.assign(nullptr, ARRAY, DIMENSIONLESS);

		cur_parent = n;
	
		cur_array_indx.push_back(0);
	
		return (int)n->children.size();
	}
	void end_array () {
		int length = cur_array_indx.back();
		cur_array_indx.pop_back();
	
		get_children(cur_parent)->resize(length); // get rid of nodes not found by find_array_node() (array length change, wrong nodes because array was not an array before, etc.)
	
		end();
	}
	
	template <typename T>
	void begin_array (cstr name, std::vector<T>* arr) {
		auto len = begin_array(name);
		if (len != -1)
			arr->resize(len);
	}

};

//
void _to_rapid_xml_recurse (rapidxml::xml_document<>* doc, rapidxml::xml_node<>* parent_xml_node, Node const& node) {

	auto* value_str = node.val.is_container() ? "" : doc->allocate_string( node.val.print().c_str() );

	auto xml_node = doc->allocate_node(rapidxml::node_type::node_element, node.name.c_str(), value_str);
	parent_xml_node->append_node(xml_node);

	auto type_attr = doc->allocate_attribute("type", typename_(node.val.get_type()));
	xml_node->append_attribute(type_attr);

	for (auto& n : node.children)
		_to_rapid_xml_recurse(doc, xml_node, *n);
}
bool to_xml (Options_Graph const& graph, cstr filepath) {

	rapidxml::xml_document<>	xml_doc;

	for (auto& n : graph.root_children)
		_to_rapid_xml_recurse(&xml_doc, &xml_doc, *n);

	struct Print_Iterator {
		std::string*	text;
		int				ptr;

		Print_Iterator (std::string* text_) {
			text = text_;
			text->assign(1, '\0');
			ptr = 0;
		}

		// these increment operators invalidate the result of operator* (dereference), but unincremented versions of Print_Iterator are still valid
		// increment operators allocate enough space in the text (with '\0'), so that a dereference on the resulting Iterator is always valid, NOTE: if the user does a *it++ = for the last char, there will be two null terminators
		Print_Iterator operator++ (int) {
			auto ret = *this; // copy this unincremented

			text->push_back('\0');
			++ptr;

			return ret;
		}
		Print_Iterator operator++ () {
			text->push_back('\0');
			++ptr;

			return *this;
		}

		char& operator* () const { // dereference
			return (*text)[ptr];
		}
	};

	std::string text;
	Print_Iterator it(&text);

	rapidxml::print(it, xml_doc, 0);

	if (text.size() > 0 && text[text.size() -1] == '\0')
		text.resize(text.size() -1); // remove redundant null terminator

	return write_text_file((filepath +std::string(".xml")).c_str(), text);
}

void _from_rapid_xml_recurse (rapidxml::xml_node<> const& node, Node* parent, std::vector< unique_ptr<Node> >* children) {
	for (auto* cur = node.first_node();; cur = cur->next_sibling()) {

		if (cur->type() == rapidxml::node_element) {

			auto n = make_unique<Node>();
			n->name = std::string(cur->name(), cur->name_size());
			n->parent = parent;

			auto val_str = std::string(cur->value(), cur->value_size());

			auto attr = cur->first_attribute("type");
			
			type_e type = NULL_;
			if (attr) type = type_from_typename(std::string(attr->value(), attr->value_size()));

			n->val.parse_from(val_str, attr, Value::DIMENSIONLESS);

			if (cur->first_node())
				_from_rapid_xml_recurse(*cur, n.get(), &n->children);

			children->emplace_back( std::move(n) );

		}
		if (cur == node.last_node())
			break;
	}
}
bool from_xml (Options_Graph* graph, cstr filepath) {

	std::string text;
	if (!load_text_file((filepath +std::string(".xml")).c_str(), &text))
		return false;

	rapidxml::xml_document<>	xml_doc;

	try {
		xml_doc.parse<0>(&text[0]);
	} catch (rapidxml::parse_error e) {
		fprintf(stderr, "rapidxml: \"%s\"\n", e.what());
		return false;
	}

	assert(graph->cur_parent == nullptr);
	graph->root_children.clear(); // deletes all nodes

	if (xml_doc.first_node())
		_from_rapid_xml_recurse(xml_doc, graph->cur_parent, &graph->root_children);

	return true;
}

//
}
}
