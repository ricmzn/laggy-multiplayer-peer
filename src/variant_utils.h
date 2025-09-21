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

#pragma once

#include <godot_cpp/variant/variant.hpp>

#define SNAME(name) []() { static StringName _name = StringName(name, true); return _name; }()

using namespace godot;

typedef CowData<void*>::Size Size;

class VariantUtils {
	static void log_call_error(const Callable &p_callable, const Variant **p_argptrs, int p_argcount, const GDExtensionCallError &p_call_error);

public:
	template <typename... Args>
	static Variant call(const Callable &p_callable, Error &out_error, Args... args) {
		Variant wrapped_callable = Variant(p_callable);
		std::array<Variant, sizeof...(args)> vargs = { args... };
		std::array<const Variant *, sizeof...(args)> argptrs;
		for (size_t i = 0; i < vargs.size(); i++) {
			argptrs[i] = &vargs[i];
		}

		Variant result;
		GDExtensionCallError call_error;
		wrapped_callable.callp(SNAME("call"), argptrs.data(), argptrs.size(), result, call_error);
		if (call_error.error == GDEXTENSION_CALL_OK) {
			out_error = OK;
		} else {
			log_call_error(p_callable, argptrs.data(), argptrs.size(), call_error);
			out_error = FAILED;
		}
		return result;
	}
};
