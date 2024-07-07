-- set project name
set_project("vgfw")

-- set project version
set_version("0.1.0")

-- set language version: C++ 20
set_languages("cxx20")

-- global options
option("examples") -- build examples?
    set_default(true)
option_end()

-- if build on windows
if is_plat("windows") then
    add_cxxflags("/EHsc")
    if is_mode("debug") then
        set_runtimes("MDd")
        add_links("ucrtd")
    else
        set_runtimes("MD")
    end
else
    add_cxxflags("-fexceptions")
end

add_rules("mode.debug", "mode.release")
add_rules("plugin.vsxmake.autoupdate")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

-- add my own xmake-repo here
add_repositories("my-xmake-repo https://github.com/zzxzzk115/xmake-repo.git dev")

-- add requirements
add_requires("vulkansdk", "vulkan-memory-allocator", "glfw", "glm", "spdlog", "stb")

-- target defination, name: vgfw
target("vgfw")
    -- set target kind: header-only
    set_kind("headeronly")

    add_includedirs(".", { public = true })

    -- add header files
    add_headerfiles("vgfw.hpp")

    add_rules("utils.install.cmake_importfiles")
    add_rules("utils.install.pkgconfig_importfiles")

    -- add packages
    add_packages("vulkansdk", { public = true })
    add_packages("vulkan-memory-allocator", { public = true })
    add_packages("glfw", { public = true })
    add_packages("glm", { public = true })
    add_packages("spdlog", { public = true })
    add_packages("stb", { public = true })

-- if build examples, then include examples
if has_config("examples") then
    includes("examples")
end
