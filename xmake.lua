
add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

package("mossui")
    set_kind("library")
    add_urls("https://github.com/Kashifro/MossUI.git")
    add_versions("1.0.0", "f36eaa350fd312029935c99f99726d5e2b38045e")
    add_includedirs("include")
    add_linkdirs("lib")
    add_links("MossUI")
    on_install(function (package)
        import("package.tools.cmake").install(package, {"-DMOSSUI_BUILD_TESTS=OFF"})
    end)
package_end()

add_requires("levilamina 26.51.7", {configs = {target_type = "client"}})

add_requires("levibuildscript")
add_requires("mossui 1.0.0")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("ModMenu")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker", {modVersion = "1.0.1-mc26.51"})

    if is_plat("windows") then
        add_defines(
            "NOMINMAX",
            "UNICODE",
            "_HAS_CXX23=1",
            "WIN32_LEAN_AND_MEAN"
        )

        add_cxflags(
            "/EHa", "/utf-8", "/W4", "/w44265", "/w44289",
            "/w44296", "/w45263", "/w44738", "/w45204"
        )

        add_cxflags(
            "/EHs",
            "-Wno-microsoft-cast",
            "-Wno-invalid-offsetof",
            "-Wno-c++2b-extensions",
            "-Wno-microsoft-include",
            "-Wno-overloaded-virtual",
            "-Wno-ignored-qualifiers",
            "-Wno-missing-field-initializers",
            "-Wno-missing-designated-field-initializers",
            "-Wno-potentially-evaluated-expression",
            "-Wno-pragma-system-header-outside-header",
            { tools = {"clang-cl"} }
        )

        set_toolchains("clang-cl")
        add_syslinks("user32", "shell32", "advapi32")
    end

    add_packages("levilamina", "mossui")

    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")

    add_files("src/**.cpp")
    add_includedirs("src")

    after_build(function (target)
        local outputdir = path.join(os.projectdir(), "bin", target:name())
        local packRoot = path.join(outputdir, "resource_packs")
        local packdir = path.join(packRoot, "ModMenu")
        local assetdir = path.join(outputdir, "assets")
        os.rm(packRoot)
        os.rm(assetdir)
        os.mkdir(packdir)
        os.mkdir(assetdir)
        os.cp(path.join(os.projectdir(), "resource_packs", "*"), packdir)
        os.cp(path.join(os.projectdir(), "assets", "*"), assetdir)
        local icon = path.join(os.projectdir(), "icon.png")
        os.cp(icon, outputdir)
    end)

