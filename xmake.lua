-- minimum xmake version
set_xmakever("2.8.2")

add_repositories("libxse-xrepo https://github.com/libxse/libxse-xrepo")

-- dependencies
includes("../commonlibob64")
includes("../BGSScriptExtenderPluginTools")

set_project("FastTravelSigns")
set_version("1.0.0")

set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

set_symbols("debug")

add_rules("mode.debug", "mode.releasedbg")

target("FastTravelSigns")

    set_kind("shared")

    add_deps("commonlibob64")
    add_deps("BGSScriptExtenderPluginTools")

    add_files("src/**.cpp")
    add_headerfiles("include/**.h")
    add_includedirs("include")

    local gamePath = os.getenv("XSE_TES4_GAME_PATH")
    if (gamePath) then
        add_extrafiles(path.join(gamePath, "OblivionRemastered/Binaries/Win64/OBSE/Plugins"))
    end

    -- flags

    add_cxxflags("/EHsc", "/permissive-")

    add_cxxflags(
        "cl::/bigobj",
        "cl::/cgthreads8",
        "cl::/diagnostics:caret",
        "cl::/external:W0",
        "cl::/fp:contract",
        "cl::/fp:except-",
        "cl::/guard:cf-",
        "cl::/Zc:preprocessor",
        "cl::/Zc:templateScope"
    )

    -- add flags (cl: disable warnings)
    add_cxxflags(
        "cl::/wd4200", -- nonstandard extension used : zero-sized array in struct/union
        "cl::/wd4201", -- nonstandard extension used : nameless struct/union
        "cl::/wd4324"  -- structure was padded due to alignment specifier
    )

    -- add flags (cl: warnings -> errors)
    add_cxxflags(
        "cl::/we4715" -- not all control paths return a value
    )

    after_build(function(target)

        import("core.project.depend")

        depend.on_changed(function()

            local pluginsDir = path.join(os.getenv("XSE_TES4_GAME_PATH"), "OblivionRemastered/Binaries/Win64/OBSE/Plugins")
            os.cp(target:targetfile(), pluginsDir)
            os.cp(string.gsub(target:targetfile(), ".dll", ".pdb"), pluginsDir)

        end, { changed = target:is_rebuilt(), files = { target:targetfile() } })

    end)
