-- target defination, name: 02-cube
target("02-cube")
    -- set target kind: executable
    set_kind("binary")

    -- add source files
    add_files("main.cpp")

    -- add deps
    add_deps("vgfw")

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/examples/02-cube")

    -- copy assets
    on_load(function(target)
        os.cp("$(scriptdir)/assets", target:targetdir())
    end)