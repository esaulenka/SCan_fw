
import qbs

CppApplication {

    name: "SCan-boot"

    // defines
    cpp.defines: [
        "STM32F10X_CL",
        "STM32F105xC",

        "BOARD_SIGMA=1",    // Sigma 10 module
        "BOARD_2CAN=2",     // Ancient 2CAN board
        "BOARD_CSAT=3",     // Custom CS device
        "BOARD_2CAN30=4",   // 2CAN 30 module
        "BOARD_2CAN2LIN=5"  // 2CAN-2LIN board
        // build for various boards
        "BOARD=1",
    ]


    files: [
        "system/startup.c",
        "system/sysinit.cpp",
        "system/vectable.c",
        "Rtt/SEGGER_RTT.c",
        "Rtt/SEGGER_RTT_printf.c",
        "USB/dfu_descriptors.c",
        "USB/usb.c",
        "USB/usb_control.c",
        "USB/usb_f107.c",
        "USB/usb_standard.c",
        "USB/usb_dwc_common.c",
        "USB/*.h",
        "boot_main.cpp",
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
        files: [ "system/stm32F105xC_boot.ld" ]
    }
    cpp.libraryPaths: [ "system" ]


    type: ["hex", "size"]         // The filetags to generate
    cpp.cLanguageVersion: "c11"
    cpp.cxxLanguageVersion: "c++17"
    cpp.positionIndependentCode: false
    cpp.generateLinkerMapFile: true
    cpp.enableExceptions: false
    cpp.executableSuffix: ".elf"
    cpp.enableRtti: false

    cpp.driverFlags: [
        "-mcpu=cortex-m3",
        "-mfloat-abi=soft",
        "-specs=nosys.specs",
        "-fdata-sections",
        "-ffunction-sections",
    ]
    cpp.cxxFlags: [
        "-fno-use-cxa-atexit",
        "-fno-exceptions",
        "-fno-rtti",
    ]
    cpp.driverLinkerFlags: [
        "-nostartfiles",
        "-specs=nano.specs",
        "-Wl,--gc-sections",
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
            var cmd = new Command(product.cpp.objcopyPath, args);
            cmd.description = "converting to hex";
            return cmd;
        }
    }

    // print size
    Rule {
        inputs: "application"
        outputFileTags: [ "size" ]
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
