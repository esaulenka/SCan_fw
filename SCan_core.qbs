
import qbs

CppApplication {

    name: "SCan-core"

    // defines
    cpp.defines: [
        "STM32F10X_CL",
        "STM32F105xC",
    ]


    files: [
        "Can/can.h",
        "Can/candrv.cpp",
        "Can/candrv.h",
        "canhacker.cpp",
        "canhacker.h",
        "system/startup.c",
        "system/sysinit.cpp",
        "system/vectable.c",
        "system/syscalls.c",
        "Rtt/SEGGER_RTT.c",
        "Rtt/SEGGER_RTT_printf.c",
        "USB/usb.c",
        "USB/usb_control.c",
        "USB/usb_f107.c",
        "USB/usb_standard.c",
        "USB/usb_dwc_common.c",
        "USB/cdcacm.cpp",
        "main.cpp",
        "timer.cpp",
        "Pins.h",
    ]


    cpp.includePaths: [
        ".",
        "system",
        "system/CMSIS",
        "USB",
        "Rtt",
        "stm32tpl",
    ]


    Group {
        name: "Linker Script"
        fileTags: ["linkerscript"]
        files: [ "system/stm32F105xC.ld" ]
    }
    cpp.libraryPaths: [ "system" ]


    type: "hex"         // The filetag to generate
    cpp.cLanguageVersion: "c11"
    cpp.cxxLanguageVersion: "c++17"
    cpp.positionIndependentCode: false
    cpp.generateLinkerMapFile: true
    cpp.enableExceptions: false
    cpp.executableSuffix: ".elf"


    cpp.driverFlags: [
        "-mcpu=cortex-m3",
        "-mfloat-abi=soft",
        "-specs=nosys.specs",
    ]
    cpp.driverLinkerFlags: [
        "-nostartfiles",
    ]


    // make hex-file
    Rule {
        inputs: "application"
        Artifact {
            fileTags: ["hex"]
            filePath: product.name + ".hex"
        }
        prepare: {
            var args = ["-O", "ihex", input.filePath, output.filePath];
            var extractorPath = product.cpp.toolchainInstallPath + "/" + product.cpp.toolchainPrefix + "objcopy";
            var cmd = new Command(extractorPath, args);
            cmd.description = "converting to hex";
            return cmd;
        }
    }

    // print size
    Rule {
        inputs: "application"
        outputFileTags: [ "siz" ]
        prepare: {
            var args = [input.filePath];
            var sizePath = product.cpp.toolchainInstallPath + "/" + product.cpp.toolchainPrefix + "size";
            var cmd = new Command(sizePath, args);
            cmd.description = "print size";
            return cmd;
        }
    }

    Properties {
        condition: qbs.buildVariant === "debug"
        cpp.debugInformation: true
        cpp.defines: outer.concat("DEBUG")
        //cpp.optimization: "none"
        cpp.driverFlags: outer.concat("-Og")
    }
    Properties {
        condition: qbs.buildVariant === "release"
        cpp.debugInformation: false
        //cpp.optimization: "small"
        cpp.driverFlags: outer.concat("-Os")
        cpp.driverLinkerFlags: outer.concat("-flto")
    }

}
