import qbs.File
import qbs.FileInfo

Project {
    name: "Unit test & helper products"

    QtcProduct {
        name: "Unit test"
        condition: (qtc_gtest_gmock.hasRepo || qtc_gtest_gmock.externalLibsPresent)
                   && QmlDesigner.present

        type: ["application", "autotest"]
        consoleApplication: true
        destinationDirectory: FileInfo.joinPaths(project.buildDirectory,
                FileInfo.relativePath(project.ide_source_tree, sourceDirectory))
        install: false

        Depends { name: "libclang"; required: false }
        Depends { name: "clang_defines" }

        Depends { name: "QmlDesigner"; required: false }

        Depends { name: "sqlite_sources" }
        Depends { name: "Core" }
        Depends { name: "CPlusPlus" }
        Depends { name: "yaml-cpp" }

        Depends { name: "Qt"; submodules: ["network", "widgets", "testlib"] }

        Depends { name: "pkgconfig"; required: false }
        Depends { name: "benchmark"; required: false }

        Depends { name: "qtc_gtest_gmock"; required: false }

        sqlite_sources.buildSharedLib: false

        cpp.defines: {
            var defines = [
                        "QT_NO_CAST_TO_ASCII",
                        "QT_RESTRICTED_CAST_FROM_ASCII",
                        "QT_USE_FAST_OPERATOR_PLUS",
                        "QT_USE_FAST_CONCATENATION",
                        "CLANGTOOLS_STATIC_LIBRARY",
                        "CPPEDITOR_STATIC_LIBRARY",
                        "DEBUGGER_STATIC_LIBRARY",
                        "UNIT_TESTS",
                        "DONT_CHECK_MESSAGE_COUNTER",
                        'QTC_RESOURCE_DIR="' + path + "/../../../share/qtcreator" + '"',
                        'TESTDATA_DIR="' + FileInfo.joinPaths(sourceDirectory, "data") + '"',
                        'RELATIVE_DATA_PATH="' + FileInfo.relativePath(destinationDirectory,
                                                                       FileInfo.joinPaths(project.sourceDirectory, "share", "qtcreator")) + '"',
                    ];
            if (libclang.present) {
                defines.push("CLANG_UNIT_TESTS");
            }
            var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                    qtc.ide_libexec_path);
            var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
            defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
            return defines;
        }
        cpp.cxxFlags: {
            var flags = [];
            if (qbs.toolchain.contains("msvc"))
                flags.push("-w34100", "/bigobj", "/wd4267", "/wd4141", "/wd4146");
            if (qbs.toolchain.contains("gcc") && !qbs.toolchain.contains("clang"))
                flags.push("-Wno-noexcept-type");
            if (qbs.toolchain.contains("clang")) {
                flags.push("-Wno-inconsistent-missing-override", "-Wno-self-move",
                           "-Wno-self-assign-overloaded");
                flags.push("-Wno-unused-command-line-argument"); // gtest puts -lpthread on compiler command line
                if (!qbs.hostOS.contains("darwin")
                        && Utilities.versionCompare(cpp.compilerVersion, "10") >= 0) {
                    flags.push("-Wno-deprecated-copy", "-Wno-constant-logical-operand");
                }
            }
            if (qbs.toolchain.contains("gcc"))
                flags.push("-Wno-unused-parameter");
            return flags;
        }
        cpp.cxxLanguageVersion: "c++17"
        cpp.dynamicLibraries: {
            var libs = [];
            if (libclang.present) {
                libs = libs.concat(libclang.llvmLibs);
                if (libclang.llvmFormattingLibs.length
                        && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)) {
                    libs = libs.concat(libclang.llvmFormattingLibs);
                }
            }
            return libs;
        }
        cpp.includePaths: {
            var paths = [
                        ".",
                        "../mockup",
                        "../../../src/libs",
                        "../../../src/libs/3rdparty",
                        "../../../src/plugins",
                        "../../../src/plugins/clangcodemodel",
                        "../../../share/qtcreator/qml/qmlpuppet/types",
                    ];
            if (libclang.present) {
                paths.push(libclang.llvmIncludeDir);
            }
            return paths;
        }
        cpp.libraryPaths: {
            var paths = [];
            if (libclang.present)
                paths.push(libclang.llvmLibDir);
            return paths;
        }
        cpp.rpaths: {
            var paths = [
                        FileInfo.joinPaths(project.buildDirectory, qtc.ide_library_path),
                        FileInfo.joinPaths(project.buildDirectory, qtc.ide_plugin_path)
                    ];
            if (libclang.present)
                paths.push(libclang.llvmLibDir);
            return paths;
        }

        files: [
            "compare-operators.h",
            "conditionally-disabled-tests.h",
            "createtablesqlstatementbuilder-test.cpp",
            "dynamicastmatcherdiagnosticcontainer-matcher.h",
            "eventspy.cpp",
            "eventspy.h",
            "fakeprocess.cpp",
            "fakeprocess.h",
            "google-using-declarations.h",
            "googletest.h",
            "gtest-creator-printing.cpp",
            "gtest-creator-printing.h",
            "gtest-llvm-printing.h",
            "gtest-qt-printing.cpp",
            "gtest-qt-printing.h",
            "matchingtext-test.cpp",
            "mockfutureinterface.h",
            "mockmutex.h",
            "mockqfilesystemwatcher.h",
            "mocksqlitestatement.h",
            "mocksqlitetransactionbackend.h",
            "mocksyntaxhighligher.h",
            "mocktimer.cpp",
            "mocktimer.h",
            "processevents-utilities.cpp",
            "processevents-utilities.h",
            "sizedarray-test.cpp",
            "smallstring-test.cpp",
            "sourcerangecontainer-matcher.h",
            "spydummy.cpp",
            "spydummy.h",
            "sqlitecolumn-test.cpp",
            "sqlitedatabase-test.cpp",
            "sqlitedatabasebackend-test.cpp",
            "sqlitedatabasemock.h",
            "sqliteindex-test.cpp",
            "sqlitereadstatementmock.cpp",
            "sqlitereadstatementmock.h",
            "sqlitestatement-test.cpp",
            "sqlitetable-test.cpp",
            "sqliteteststatement.h",
            "sqlitetransaction-test.cpp",
            "sqlitevalue-test.cpp",
            "sqlitewritestatementmock.cpp",
            "sqlitewritestatementmock.h",
            "sqlstatementbuilder-test.cpp",
            "unittest-utility-functions.h",
            "unittests-main.cpp",
        ]

        Group {
            name: "libclang tests"
            condition: libclang.present && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)
            files: [
                "activationsequenceprocessor-test.cpp",
                "clangcompareoperators.h",
                "diagnosticcontainer-matcher.h",
                "readexporteddiagnostics-test.cpp",
            ]
        }

        Group {
            name: "benchmark test"
            condition: benchmark.present
            files: "smallstring-benchmark.cpp"
        }

        Group {
            name: "data"
            files: [
                "data/*",
                "data/include/*",
            ]
            fileTags: []
        }

        Group {
            name: "sources from clangcodemodel"
            prefix: "../../../src/plugins/clangcodemodel/"
            files: [
                "clangactivationsequenceprocessor.cpp",
                "clangactivationsequenceprocessor.h",
            ]
        }

        Group {
            name: "sources from cppeditor"
            prefix: "../../../src/plugins/cppeditor/"
            files: [
                "cppprojectfile.cpp",
                "cppprojectfile.h",
            ]
        }

        Group {
            name: "sources from clangtools"
            condition: libclang.present
            prefix: "../../../src/plugins/clangtools/"
            files: [
                "clangtoolsdiagnostic.cpp",
                "clangtoolsdiagnostic.h",
                "clangtoolslogfilereader.cpp",
                "clangtoolslogfilereader.h",
            ]
        }

        Group {
            name: "sources from Debugger"
            prefix: "../../../src/plugins/debugger/analyzer/"
            files: [
                "diagnosticlocation.cpp",
                "diagnosticlocation.h",
            ]
        }
    }
}
