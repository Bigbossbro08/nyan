// Copyright 2017-2017 the nyan authors, LGPLv3+. See copying.md for legal info.

#include "database.h"

#include <memory>
#include <unordered_map>
#include <queue>

#include "c3.h"
#include "error.h"
#include "file.h"
#include "namespace.h"
#include "object_state.h"
#include "parser.h"
#include "util.h"
#include "view.h"


namespace nyan {

Database::Database() {}


Database::~Database() {}


/**
 * Called for each object.
 * Contains the scope, current namespace,
 * name of the object, and the astobject.
 */
using ast_objwalk_cb_t = std::function<void(const NamespaceFinder &,
                                            const Namespace &,
                                            const Namespace &,
                                            const ASTObject &)>;


static void ast_obj_walk_recurser(const ast_objwalk_cb_t &callback,
                                  const NamespaceFinder &scope,
                                  const Namespace &ns,
                                  const std::vector<ASTObject> &objs) {

	// go over all objects
	for (auto &astobj : objs) {
		Namespace objname{ns, astobj.get_name().get()};

		// process nested objects first
		ast_obj_walk_recurser(callback, scope, objname, astobj.get_objects());

		// do whatever needed
		callback(scope, ns, objname, astobj);
	}
}


static void ast_obj_walk(const namespace_lookup_t &imports,
                         const ast_objwalk_cb_t &cb) {

	// go over all the imported files
	for (auto &it : imports) {
		const Namespace &ns = it.first;
		const NamespaceFinder &current_file = it.second;
		const AST &ast = current_file.get_ast();

		// each file has many objects, which can be nested.
		ast_obj_walk_recurser(cb, current_file, ns, ast.get_objects());
	}
}


void Database::load(const std::string &filename,
                    const filefetcher_t &filefetcher) {

	Parser parser;

	// tracking of imported namespaces (with aliases)
	namespace_lookup_t imports;

	// namespaces to which were requested to be imported
	// the location is the first request origin.
	std::unordered_map<Namespace, Location> to_import;

	// push the first namespace to import
	to_import.insert(
		{
			Namespace::from_filename(filename),
			Location{"explicit load request"}
		}
	);

	while (to_import.size() > 0) {
		auto cur_ns_it = to_import.begin();
		const Namespace &namespace_to_import = cur_ns_it->first;
		const Location &req_location = cur_ns_it->second;

		std::cout << "loading file " << namespace_to_import.to_filename() << std::endl;

		auto it = imports.find(namespace_to_import);
		if (it != std::end(imports)) {
			// this namespace is already imported!
			continue;
		}

		std::shared_ptr<File> current_file;
		try {
			// get the data and parse the file
			current_file = filefetcher(
				namespace_to_import.to_filename()
			);
		}
		catch (FileReadError &err) {
			// the import request failed,
			// so the nyan file structure or content is wrong.
			throw FileError{req_location, err.str()};
		}

		// create import tracking entry for this file
		// and parse the file contents!
		NamespaceFinder &new_ns = imports.insert({
			namespace_to_import,                         // name of the import
			NamespaceFinder{
				parser.parse(current_file)  // read the ast!
			}
		}).first->second;

		// enqueue all new imports of this file
		// and record import aliases
		for (auto &import : new_ns.get_ast().get_imports()) {
			Namespace request{import.get()};

			// either register the alias
			if (import.has_alias()) {
				new_ns.add_alias(import.get_alias(), request);
			}
			// or the plain import
			else {
				new_ns.add_import(request);
			}

			// check if this import was already requested or is known.
			// todo: also check if that ns is already fully loaded in the db
			auto was_imported = imports.find(request);
			auto import_requested = to_import.find(request);

			if (was_imported == std::end(imports) and
			    import_requested == std::end(to_import)) {

				// add the request to the pending imports
				to_import.insert({std::move(request), import.get()});
			}
		}

		to_import.erase(cur_ns_it);
	}


	for (auto &it : imports) {
		std::cout << it.first.str() << " has known info:" << std::endl
		          << it.second.str() << std::endl;
	}

	using namespace std::placeholders;

	size_t new_obj_count = 0;

	// first run: create empty object info objects
	ast_obj_walk(imports, std::bind(&Database::create_obj_info,
	                                this, &new_obj_count,
	                                _1, _2, _3, _4));

	std::vector<fqon_t> new_objects;
	new_objects.reserve(new_obj_count);

	// now, all new object infos need to be filled with types
	ast_obj_walk(imports, std::bind(&Database::create_obj_content,
	                                this, &new_objects,
	                                _1, _2, _3, _4));

	// linearize the parents of all new objects
	this->linearize_new(new_objects);

	// resolve the types of members to their definition
	this->resolve_types(new_objects);


	// state value creation
	ast_obj_walk(imports, std::bind(&Database::create_obj_state,
	                                this,
	                                _1, _2, _3, _4));

	// TODO asdf member values.

	// TODO: create value by type
	// TODO: create check operation for value

	/*
	  ast
	    x import[]
	    x   namespace_name
	    x   alias
	      object[]
	    x   name
	    t   target
	    t   inheritance_add[]
	    s   parents[]
	    x   member[]
	    x     name
	    s     operation
	    t     type
	            does_exist
	    t       name
	            has_payload
	    t       payload
	    s     value
	            does_exist
	            container_type
	    s       values[]
	    ... objects[]

	  typetree
	    name -> objectinfo
	      location
	      target                     (objects)
	      inheritance_add[]          (objects)
	      memberid -> memberinfo
	        location
	        type                     (objects)

	  statetree
	    name -> objectstate
	      memberid -> member
	        override_depth
	        operation
	        value                    (objects)
	      parents[]
	        fqon_t                   (objects) [linearize after setting]


	  * The patch will fail to be loaded if:
	  * The patch target is not known
	  * Any of changed members is not present in the patch target
	  * Any of the added parents is not known
	  * -> Blind patching is not allowed
	  * The patch will succeed to load if:
	  * The patch target already inherits from a parent to be added
	  * -> Inheritance patching doesn't conflict with other patches
	 */

	std::cout << std::endl << "METAINFO:" << std::endl
	          << this->meta_info.str() << std::endl;

	std::cout << std::endl << "INITIAL STATE:" << std::endl
	          << this->state.str() << std::endl;

	// TODO: check pending objectvalues (probably not needed as they're all loaded)
}


void Database::create_obj_info(size_t *counter,
                               const NamespaceFinder &current_file,
                               const Namespace &ns,
                               const Namespace &objname,
                               const ASTObject &astobj) {

	const std::string &name = astobj.name.get();

	// object name must not be an alias
	if (current_file.check_conflict(name)) {
		// TODO: show conflict origin (the import)
		throw NameError{
			astobj.name,
			"object name conflicts with import",
			name
		};
	}

	ObjectInfo &info = this->meta_info.add_object(
		objname.to_fqon(),
		ObjectInfo{astobj.name}
	);

	*counter += 1;
}


void Database::create_obj_content(std::vector<fqon_t> *new_objs,
                                  const NamespaceFinder &scope,
                                  const Namespace &ns,
                                  const Namespace &objname,
                                  const ASTObject &astobj) {

	fqon_t obj_fqon = objname.to_fqon();
	new_objs->push_back(obj_fqon);

	ObjectInfo *info = this->meta_info.get_object(obj_fqon);
	if (unlikely(info == nullptr)) {
		throw InternalError{"object info could not be retrieved"};
	}

	// save the patch target, has to be alias-expanded
	const IDToken &target = astobj.target;
	if (target.exists()) {
		fqon_t target_id = scope.find(ns, target, this->meta_info);
		info->set_target(std::move(target_id));
	}

	// a patch may add inheritance parents
	for (auto &new_parent : astobj.inheritance_add) {
		fqon_t new_parent_id = scope.find(ns, new_parent, this->meta_info);
		info->add_inheritance_add(std::move(new_parent_id));
	}

	// parents are stored in the object data state
	std::vector<fqon_t> object_parents;
	for (auto &parent : astobj.parents) {
		fqon_t parent_id = scope.find(ns, parent, this->meta_info);
		object_parents.push_back(std::move(parent_id));
	}

	// fill initial state:
	this->state.add_object(
		obj_fqon,
		std::make_shared<ObjectState>(
			std::move(object_parents)
		)
	);

	// create member types
	for (auto &astmember : astobj.members) {

		// TODO: the member name requires advanced expansions
		//       for conflict resolving

		MemberInfo &member_info = info->add_member(
			astmember.name.str(),
			MemberInfo{astmember.name}
		);

		if (not astmember.type.exists()) {
			continue;
		}

		// if existing, create type information of member.
		member_info.set_type(
			std::make_shared<Type>(
				astmember.type,
				scope,
				ns,
				this->meta_info
			),
			true   // type was defined in the ast -> initial definition
		);
	}
}


void Database::linearize_new(const std::vector<fqon_t> &new_objects) {
	// linearize the parents of all newly created objects
	std::unordered_set<fqon_t> linearized_objects;

	for (auto &obj : new_objects) {

		if (linearized_objects.find(obj) != std::end(linearized_objects)) {
			continue;
		}

		std::unordered_set<fqon_t> seen;

		linearize_recurse(
			obj,
			[this] (const fqon_t &name) -> ObjectState& {
				return *this->state.get(name);
			},
			&seen
		);

#if __cplusplus > 201402L  // c++17
		linearized_objects.merge(std::move(seen));
#else
		linearized_objects.insert(std::begin(seen), std::end(seen));
#endif
	}
}


void Database::resolve_types(const std::vector<fqon_t> &new_objects) {

	using namespace std::string_literals;

	// resolve member types:
	// link member types to matching parent if not known yet.
	for (auto &obj : new_objects) {
		ObjectInfo *obj_info = this->meta_info.get_object(obj);
		ObjectState *obj_state = this->state.get(obj).get();

		// check if each member has a type
		for (auto &it : obj_info->get_members()) {
			const memberid_t &member_id = it.first;
			MemberInfo &member_info = it.second;

			// type for this member is needed
			bool type_needed = true;

			// member has a type and it's defined as initial def
			// -> no parent should define it.
			if (member_info.is_initial_def()) {
				type_needed = false;
			}

			// TODO: figure out a type from a patch target
			// when? after top parent has been reached?

			// member doesn't have type yet. find it.
			const auto &parents_lin = obj_state->get_linearization();

			auto parent = std::begin(parents_lin);
			// first parent is always the object itself, skip it.
			++parent;
			for (auto end=std::end(parents_lin); parent != end; ++parent) {
				ObjectInfo *parent_info = this->meta_info.get_object(*parent);
				const MemberInfo *parent_member_info = parent_info->get_member(member_id);

				// parent doesn't have this member
				if (not parent_member_info) {
					continue;
				}

				if (parent_member_info->is_initial_def()) {
					const std::shared_ptr<Type> &new_type = parent_member_info->get_type();

					if (unlikely(not new_type.get())) {
						throw InternalError{"initial type definition has no type"};
					}
					else if (unlikely(type_needed == false)) {
						// another parent defines this type,
						// which is disallowed.

						// TODO: show location of type instead of member
						throw TypeError{
							member_info.get_location(),
							("parent '"s + *parent
							 + "' already defines type of '" + member_id + "'")
						};
					}

					type_needed = false;
					member_info.set_type(new_type, false);
				}
			}

			if (unlikely(type_needed)) {
				throw TypeError{
					member_info.get_location(),
					"no parent defines the type of '"s + member_id + "'"
				};
			}
		}
	}
}


void Database::create_obj_state(const NamespaceFinder &scope,
                                const Namespace &ns,
                                const Namespace &objname,
                                const ASTObject &astobj) {

	if (astobj.members.size() == 0) {
		// no members, nothing to do.
		return;
	}

	ObjectInfo *info = this->meta_info.get_object(objname.to_fqon());
	if (unlikely(info == nullptr)) {
		throw InternalError{"object info could not be retrieved"};
	}

	ObjectState &objstate = *this->state.get(objname.to_fqon());

	std::unordered_map<memberid_t, Member> members;

	// create member values
	for (auto &astmember : astobj.members) {

		// member has no value
		if (not astmember.value.exists()) {
			continue;
		}

		// TODO: the member name may need some resolution for conflicts
		const memberid_t &memberid = astmember.name.str();

		const MemberInfo *member_info = info->get_member(memberid);
		if (unlikely(member_info == nullptr)) {
			throw InternalError{"member info could not be retrieved"};
		}

		const Type *member_type = member_info->get_type().get();
		if (unlikely(member_type == nullptr)) {
			throw InternalError{"member type could not be retrieved"};
		}

		nyan_op operation = astmember.operation;

		if (unlikely(operation == nyan_op::INVALID)) {
			// the ast buildup should have ensured this.
			throw InternalError{"member has value but invalid operator"};
		}

		// create the member with operation and value
		Member &new_member = members.insert({
			memberid,
			Member{
				0,          // TODO: get override depth from AST (the @-count)
				operation,
				Value::from_ast(*member_type, astmember.value, scope, ns, this->meta_info)
			}
		}).first->second;

		const Value &new_value = new_member.get_value();

		const std::unordered_set<nyan_op> &allowed_ops = new_value.allowed_operations(*member_type);

		if (allowed_ops.find(operation) == std::end(allowed_ops)) {
			// TODO: show location of operation
			throw TypeError{astmember.name, "invalid operator for member type"};
		}
	}

	objstate.set_members(std::move(members));
}


// TODO: check if operator is allowed for the value type.
// TODO: sanity check for inheritance operator compat (no += on undef parent)


std::shared_ptr<View> Database::new_view() {
	return std::make_shared<View>(shared_from_this());
}


} // namespace nyan
