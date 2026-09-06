#import "Basic";
#import "Compiler";
#import "File_Utilities"; // file_exists()
#import "Process";

// The tutorial we follow writes its shaders in Slang and compiles them at runtime
// through the Slang C API. We do the same thing with the stand-alone compiler that
// ships with the Vulkan SDK instead, so that the program itself contains no shader
// compiler and just loads a finished SPIR-V file.
SHADER_SOURCE :: "assets/shaders/triangle.slang";
SHADER_OUTPUT :: "assets/shaders/triangle.spv";

// Checked in order; the last resort is to assume slangc is on PATH.
SLANGC_CANDIDATES :: string.[
    "C:/VulkanSDK/1.4.341.1/Bin/slangc.exe",
];

#run {
    set_build_options_dc(.{do_output=false});

    if !compile_shaders() {
        compiler_set_workspace_status(.FAILED);
        return;
    }

    w := compiler_create_workspace("Builder");
    if !w {
        log_error("The program is not ready!");
        compiler_set_workspace_status(.FAILED);
        return;
    }

    options := get_build_options(w);
    options.output_executable_name = "engine";
    set_build_options(options, w);

    add_build_file("src/main.jai", w);
}

compile_shaders :: () -> bool {
    slangc := find_slangc();

    // Both entry points live in one .slang file, so a single invocation produces a
    // single SPIR-V module that holds a vertex stage and a fragment stage.
    command: [] string = .[
        slangc, SHADER_SOURCE,
        "-target",  "spirv",
        "-profile", "spirv_1_4",
        "-emit-spirv-directly",
        "-entry", "vertex_main",   "-stage", "vertex",
        "-entry", "fragment_main", "-stage", "fragment",
        "-o", SHADER_OUTPUT,
    ];

    log("Compiling % -> %", SHADER_SOURCE, SHADER_OUTPUT);
    result, output, errors := run_command(..command, capture_and_return_output = true);

    if result.type != .EXITED || result.exit_code != 0 {
        log_error("Could not compile the shader with '%'.\n%\n%", slangc, output, errors);
        log_error("Install the Vulkan SDK (which bundles slangc) or put slangc on your PATH.");
        return false;
    }
    return true;
}

find_slangc :: () -> string {
    for SLANGC_CANDIDATES {
        if file_exists(it)  return it;
    }
    return "slangc";
}
