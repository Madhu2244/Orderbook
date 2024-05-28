
if(ON)
    list(APPEND B2_OPTIONS
        abi=ms
        binary-format=pe
    )
endif()

if()
    if(x64 MATCHES "^(ARM|arm)64$")
        list(APPEND B2_OPTIONS
            abi=aapcs
            binary-format=mach-o
        )
    else()
        list(APPEND B2_OPTIONS
            abi=sysv
            binary-format=mach-o
        )
    endif()
endif()

if("" STREQUAL "ON" OR "" STREQUAL "ON")
    if(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm" OR VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
        list(APPEND B2_OPTIONS
            abi=aapcs
        )
    elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86" OR VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
        list(APPEND B2_OPTIONS
            abi=sysv
        )  
    endif()
    list(APPEND B2_OPTIONS
        binary-format=elf
    )
endif()