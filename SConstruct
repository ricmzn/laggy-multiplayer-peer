#!/usr/bin/env python
import os
import sys

from methods import print_error


libname = "laggy_multiplayer_peer"
project_dir = "demo"
godot_cpp_dir = "godot-cpp"

local_env = Environment(tools = ["default"], PLATFORM = "")

# Build profiles can be used to decrease compile times.
# You can either specify "disabled_classes", OR
# explicitly specify "enabled_classes" which disables all other classes.
# local_env["build_profile"] = "build_profile.json"

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Update(local_env)

Help(opts.GenerateHelpText(local_env))

env = local_env.Clone()

if not (os.path.isdir(godot_cpp_dir) and os.listdir(godot_cpp_dir)):
	print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

	git submodule update --init --recursive""")
	sys.exit(1)

env = SConscript(f"{godot_cpp_dir}/SConstruct", {"env": env, "customs": customs})

# env.Append(CXXFLAGS = ["-std=c++23"])
# env.Append(LIBS = ["stdc++exp"])

# Optional build with ubsan to detect bugs that may lead to corruption or crashes.
# Only use when debugging!
# env.Append(CXXFLAGS = ["-fsanitize=undefined"])
# env.Append(LDFLAGS = ["-fsanitize=undefined"])

env.Append(CPPPATH = ["src/"])
sources = Glob("src/*.cpp")

if env["target"] in ["editor", "template_debug"]:
	doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
	sources.append(doc_data)

# .dev doesn't inhibit compatibility, so we don't need to key it.
# .universal just means "compatible with all relevant arches" so we don't need to key it.
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst("$SHLIBPREFIX"), libname, suffix, env.subst("$SHLIBSUFFIX"))

library = env.SharedLibrary(
	"{}/bin/{}/{}".format(project_dir, env["platform"], lib_filename),
	source = sources,
)

default_args = [library]
Default(*default_args)
