"""Provides a rule that outputs a monolithic static library."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")

TOOLS_CPP_REPO = "@bazel_tools"

def _cc_static_library_impl(ctx):
    output_lib = ctx.actions.declare_file("{}.a".format(ctx.attr.name))
    output_flags = ctx.actions.declare_file("{}.link".format(ctx.attr.name))

    cc_toolchain = find_cpp_toolchain(ctx)

    print("cc_toolchain: {} ".format(cc_toolchain))

    lib_sets = []
    unique_flags = {}

    libraries_to_link = []
    for dep in ctx.attr.deps:
        cc = dep[CcInfo]
        for link_input in cc.linking_context.linker_inputs.to_list():
            libraries_to_link.extend(link_input.libraries)

    libs = []
    for lib in libraries_to_link:
        libs.append(lib.pic_static_library)

    script_file = ctx.actions.declare_file("{}.mri".format(ctx.attr.name))
    commands = ["create {}".format(output_lib.path)]
    for lib in libs:
        commands.append("addlib {}".format(lib.path))
        commands.append("save")
        commands.append("end")
        ctx.actions.write(
            output = script_file,
            content = "\n".join(commands) + "\n",
        )

    ctx.actions.run_shell(
        command = "{} -M < {}".format(cc_toolchain.ar_executable, script_file.path),
        inputs = [script_file] + libs + cc_toolchain.all_files.to_list(),
        outputs = [output_lib],
        mnemonic = "ArMerge",
        progress_message = "Merging static library {}".format(output_lib.path),
    )


    return [
        DefaultInfo(files = depset([output_flags, output_lib])),
    ]

cc_static_library = rule(
    implementation = _cc_static_library_impl,
    attrs = {
        "deps": attr.label_list(),
        "_cc_toolchain": attr.label(
            default = TOOLS_CPP_REPO + "//tools/cpp:current_cc_toolchain",
        ),
    },
    toolchains = [TOOLS_CPP_REPO + "//tools/cpp:toolchain_type"],
    incompatible_use_toolchain_transition = True,
)
