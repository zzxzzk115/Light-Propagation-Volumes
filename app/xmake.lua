add_requires("shaderc", {configs = {binaryonly = true}})

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

    -- add deps
    add_deps("vgfw")

    -- add packages
    add_packages("shaderc")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/lpv-app")