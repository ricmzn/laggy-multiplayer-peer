// Copyright (c) 2025 Ricardo Nunes
// Copyright (c) 2014-2025 Godot Engine contributors (see https://github.com/godotengine/godot/blob/master/AUTHORS.md).
// Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "variant_utils.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void VariantUtils::log_call_error(const Callable &p_callable, const Variant **p_argptrs, int p_argcount, const GDExtensionCallError &p_call_error) {
	String err_text;

	if (p_call_error.error == GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT) {
		int errorarg = p_call_error.argument;
		if (p_argptrs) {
			err_text = "Cannot convert argument " + itos(errorarg + 1) + " from " + Variant::get_type_name(p_argptrs[errorarg]->get_type()) + " to " + Variant::get_type_name(static_cast<Variant::Type>(p_call_error.expected));
		} else {
			err_text = "Cannot convert argument " + itos(errorarg + 1) + " from [missing argptr, type unknown] to " + Variant::get_type_name(static_cast<Variant::Type>(p_call_error.expected));
		}
	} else if (p_call_error.error == GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS) {
		err_text = "Method expected " + itos(p_call_error.expected) + " arguments, but called with " + itos(p_argcount);
	} else if (p_call_error.error == GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS) {
		err_text = "Method expected " + itos(p_call_error.expected) + " arguments, but called with " + itos(p_argcount);
	} else if (p_call_error.error == GDEXTENSION_CALL_ERROR_INVALID_METHOD) {
		err_text = "Method not found";
	} else if (p_call_error.error == GDEXTENSION_CALL_ERROR_INSTANCE_IS_NULL) {
		err_text = "Instance is null";
	} else if (p_call_error.error == GDEXTENSION_CALL_ERROR_METHOD_NOT_CONST) {
		err_text = "Method not const in const instance";
	} else {
		err_text = "Unexpected call error";
	}

	String base_text;
	if (Object *base = p_callable.get_object()) {
		base_text = base->get_class();
		if (Ref<Resource> script = base->get_script(); script.is_valid()) {
			base_text += "(" + script->get_path().get_file() + ")";
		}
		base_text += "::";
	}

	UtilityFunctions::push_error("'" + base_text + String(p_callable.get_method()) + "': " + err_text);
}
