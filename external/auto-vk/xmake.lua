-- add requirements
add_requires("vulkansdk")

-- target defination, name: autovk
target("autovk")
    -- set target kind: static library
    set_kind("static")

    -- add include dir
    add_includedirs("include", {public = true}) -- public: let other targets to auto include

    -- add header files
    add_headerfiles("include/(avk/**.hpp)")

    -- add source files
    add_files("src/avk.cpp")

    -- add packages
    add_packages("vulkansdk", { public = true })

    -- set target directory
    set_targetdir("$(buildir)/$(plat)/$(arch)/$(mode)/autovk")