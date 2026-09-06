// MIT License - Copyright (c) 2022 Hamad Almamari
// Tested with vulkan 1.0 and 1.2
// You can use this file to generate any version of vulkan bindings.
// You only need to set the vulkan.h and vulkan lib paths.

// config
AT_COMPILE_TIME   :: true;
vulkan_h_location :: "source/vulkan.h";

// if the lib path is empty system_library will be used.
#if OS == .WINDOWS {
    vulkan_lib_path :: "win";
}  else {
    vulkan_lib_path :: "";
}


#if AT_COMPILE_TIME {
    #run,stallable {
        set_build_options_dc(.{do_output=false});
        if !generate_bindings() {
            compiler_set_workspace_status(.FAILED);
        }
    }
} else {
    #import "System";

    main :: () {
        set_working_directory(path_strip_filename(get_path_of_running_executable()));
        if !generate_bindings() {
            exit(1);
        }
    }
}

//--------------------

OMIT_LIST :: string.[
    // These are defined manually:
    "VK_NULL_HANDLE",
    "VK_TRUE",
    "VK_FALSE",
    "VK_WHOLE_SIZE",
    "VK_ATTACHMENT_UNUSED",
    "VK_QUEUE_FAMILY_IGNORED",
    "VK_REMAINING_ARRAY_LAYERS",
    "VK_REMAINING_MIP_LEVELS",
    "VK_SUBPASS_EXTERNAL",
    "VK_MAX_PHYSICAL_DEVICE_NAME_SIZE",
    "VK_UUID_SIZE",
    "VK_MAX_MEMORY_TYPES",
    "VK_MAX_MEMORY_HEAPS",
    "VK_MAX_EXTENSION_NAME_SIZE",
    "VK_MAX_DESCRIPTION_SIZE",
    // Vulkan 1.3 functions that exist on Linux but are not part of our Windows version:
    "vkGetPhysicalDeviceSurfaceCapabilities2KHR",
    "vkGetPhysicalDeviceSurfaceFormats2KHR",
];

generate_bindings :: () -> bool {
    output_filename: string;
    options: Generate_Bindings_Options;
    {
        using options;

        #if OS == .WINDOWS {
            array_add(*libnames, "vulkan-1");
            array_add(*extra_clang_arguments, "-x", "c", "-DWIN32_LEAN_AND_MEAN");
            output_filename  = "generated_windows.jai";
        } else #if OS == .LINUX {
            array_add(*system_libraries, .{filename = "libvulkan.so.1"});
            output_filename  = "generated_linux.jai";
        } else #if OS == .MACOS {
            // @ToDo
            assert(false);
        } else {
            assert(false);
        }

        if vulkan_lib_path then array_add(*libpaths, vulkan_lib_path);

        array_add(*source_files, vulkan_h_location);

        generate_library_declarations = true;
        will_print_bindings = () {
            // Change the library name to be the same on all platforms
            // so that we don't have to generate identical bindings for every platform just because the library name is different.
            context.generator.libraries[0].identifier = "libvulkan";
        };

        visitor     = vulkan_visitor;
        log_stripped_declarations = true;
        generate_compile_time_struct_checks = false;
    }

    init_string_builder(*enumeration_wrapper_builder);

    bindings_builder, success := generate_bindings(options, log_results = true);
    if !success return false;

    append(*bindings_builder, "\n#import \"Basic\";\n\n#scope_export\n\n// Enumeration helpers\n\n");
    append_and_steal_buffers(*bindings_builder, *enumeration_wrapper_builder);

    success = write_entire_file(output_filename, *bindings_builder);
    if !success {
        log_error("Could not write generated output to \"%\"", output_filename);
        return false;
    }
    log("\nOK! generated '%'", output_filename);
    return true;
}

available_flag_bits: Table(string, *Enum);
struct_type_decl: *Enum;

enumeration_wrapper_builder: String_Builder;

vulkan_visitor :: (decl: *Declaration, parent_decl: *Declaration) -> Declaration_Visit_Result {
    // Omit header definitions (ex: VULKAN_H_)
    if decl.kind == .MACRO_DEFINITION && ends_with(decl.name, "_H_") {
        // print("MACRO_DEFINITION removed: %\n", decl.name);
        decl.decl_flags |= .OMIT_FROM_OUTPUT;
        return .STOP;
    }

    // Omit our OMIT_LIST
    if !parent_decl && array_find(OMIT_LIST, decl.name) {
        //print("OMIT_LIST: %\n", decl.name);
        decl.decl_flags |= .OMIT_FROM_OUTPUT;
        return .STOP;
    }

    if decl.kind == .ENUM {
        en := cast(*Enum)decl;
        if en.name == "VkStructureType" {
            struct_type_decl = en; // Save for later
        }

        // Almost all enums should be unsigned
        if en.type && decl.name != "VkResult" {
            if en.type.size == {
                case 1;
                    en.type = context.generator.type_def_u8;
                case 2;
                    en.type = context.generator.type_def_u16;
                case 4;
                    en.type = context.generator.type_def_u32;
                case 8;
                    en.type = context.generator.type_def_u64;
            }
        }

        if contains(decl.name, "FlagBits") {
            en.flags |= .IS_ENUM_FLAGS;
            en.flags |= .VALUES_IN_HEX;

            // Remember this so we can replace the generic VkFlags aliases with the actual enum_flag's type below. :VkFlags
            table_add(*available_flag_bits, decl.name, en);
        }
    }

    // Replace typedef which has "VkFlags" as value. :VkFlags
    // VkAccessFlags :: VkFlags   ->   VkAccessFlags :: VkAccessFlagBits;
    // We don't change the name of the FlagBits enum directly because both the ...FlagBits and the ...Flags aliases are used in declarations.
    if decl.kind == .TYPEDEF && decl.type.type_of_typedef &&
        (decl.type.type_of_typedef.name == "VkFlags" || decl.type.type_of_typedef.name == "VkFlags64") {
        flags_name := replace(decl.name, "Flags", "FlagBits");
        found, enum_decl := table_find(*available_flag_bits, flags_name);
        if found {
            type := New(CType);
            type.type_of_enum = enum_decl;
            type.size = enum_decl.type.size;
            decl.type = type;
        }
    }

    // Set the default value for sType.
    if parent_decl                              &&
       parent_decl.kind == .STRUCT              &&
       parent_decl.name != "VkBaseInStructure"  &&
       parent_decl.name != "VkBaseOutStructure" &&
       decl.kind == .DECLARATION                &&
       decl.name == "sType" {
        expected_stype_name := parent_decl.name;

        // We remove the "Vk" prefix from the expected name.
        assert(expected_stype_name.count > 2);
        expected_stype_name.data  += 2;
        expected_stype_name.count -= 2;

        enum_value: *Declaration;
        for struct_type_decl.enumerates {
            if equal_nocase_ignoring_underscores(it.output_name, expected_stype_name) {
                enum_value = it;
                break;
            }
        }

        if enum_value == null {
            log("Warning: Could not find % enum value for \"%\"", struct_type_decl.name, expected_stype_name, flags = .WARNING);
        } else {
            literal := New(Literal);
            literal.literal_kind = .ENUM;
            literal.enum_type    = struct_type_decl;
            literal.enum_value   = enum_value;

            decl.expression      = literal;
        }
    }

    if decl.kind == .FUNCTION {
        function := cast(*Function) decl;
        maybe_add_enumeration_wrapper(*enumeration_wrapper_builder, function);
    }

    return .RECURSE;
}

maybe_add_enumeration_wrapper :: (builder: *String_Builder, function: *Function) {
    if function.decl_flags & .OMIT_FROM_OUTPUT return;

    func_type := function.type.type_of_function;
    assert(func_type != null);

    if func_type.arguments.count < 2 return;

    count_arg := func_type.arguments[func_type.arguments.count - 2];
    data_arg := func_type.arguments[func_type.arguments.count - 1];

    // Make sure that the count is a *u32, that data is a pointer and that the return values is a VkResult (if present)
    if count_arg.type.pointer_to != context.generator.type_def_u32 return;
    if !data_arg.type.pointer_to return;
    assert(func_type.return_type != null);
    has_return_value := (func_type.return_type != context.generator.type_def_void);
    if has_return_value && get_type_name(func_type.return_type) != "VkResult" return;

    print_to_builder(builder, "% :: (", function.name);

    for i: 0..func_type.arguments.count - 3 {
        if i append(builder, ", ");
        arg := func_type.arguments[i];
        print_to_builder(builder, "%: ", arg.name);
        print_type_to_builder(builder, arg.type, null);
    }

    append(builder, ") -> [] ");
    print_type_to_builder(builder, data_arg.type.pointer_to, null);

    if has_return_value {
        append(builder, ", VkResult");
    }

    append(builder, " {\n");
    append(builder, "    count: u32;\n    data: [] ");
    print_type_to_builder(builder, data_arg.type.pointer_to, null);
    append(builder, ";\n");

    if has_return_value {
        append(builder, "    result: VkResult;\n");
    }

    print_call :: () #expand {
        append(builder, "    ");
        if has_return_value {
            append(builder, "result = ");
        }

        print_to_builder(builder, "%(", function.name);

        for 0..func_type.arguments.count - 3 {
            arg := func_type.arguments[it];
            print_to_builder(builder, "%, ", arg.name);
        }

        append(builder, "*count, data.data);\n");
    }

    print_call();

    if has_return_value {
        append(builder, "    if result return .[], result;\n");
    }

    append(builder, "    if count == 0 return .[]");
    if has_return_value {
        append(builder, ", .SUCCESS");
    }
    append(builder, ";\n");

    append(builder, "    data = NewArray(count, ");
    print_type_to_builder(builder, data_arg.type.pointer_to, null);
    append(builder, ", initialized = false);\n");

    print_call();

    append(builder, "    return data");
    if has_return_value {
        append(builder, ", result");
    }
    append(builder, ";\n}\n\n");
}

equal_nocase_ignoring_underscores :: (a: string, b: string) -> bool {
    a_index := 0;
    b_index := 0;
    while true {
        while a_index < a.count && a[a_index] == #char "_" { a_index += 1; }
        while b_index < b.count && b[b_index] == #char "_" { b_index += 1; }

        if (a_index >= a.count) ^ (b_index >= b.count)
            return false;
        else if a_index == a.count && b_index == b.count
            return true;

        if to_upper(a[a_index]) != to_upper(b[b_index])
            return false;

        a_index += 1;
        b_index += 1;
    }

    // This line is here just to avoid the compiler warning.
    return false;
}


#scope_file
#import "Basic";
#import "Compiler";
#import "Check";
#import "Bindings_Generator";
#import "File";
#import "String";
#import "Hash_Table";

