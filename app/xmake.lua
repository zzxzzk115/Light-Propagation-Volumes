add_requires("vgfw")
add_requires("tracy", {configs = {on_demand = true}})
add_requires("shaderc", {configs = {binaryonly = true}}) -- use glslc binary to preprocess shaders

-- target defination, name: lpv-app
target("lpv-app")
    -- set target kind: executable
    set_kind("binary")

    add_includedirs(".", { public = true })

    -- set values
    set_values("asset_files", "assets/models/Sponza/**")
    set_values("shader_root", "$(scriptdir)/shaders")

    -- add rules
    add_rules("copy_assets", "imguiconfig", "preprocess_shaders")

    -- add source files
    add_files("**.cpp")
    add_files("imgui.ini")

    -- add shaders
    add_files("shaders/**")

    -- add packages
    add_packages("vgfw", "tracy", "shaderc")

    -- add defines
    add_defines("VGFW_ENABLE_TRACY", "VGFW_ENABLE_GL_DEBUG") -- Comment this line to do the memory usage test.

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/lpv-app")