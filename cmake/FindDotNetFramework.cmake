# Find .NET Framework libraries and headers
#
# This module sets the following variables:
#   DotNetFramework_FOUND          - True if .NET Framework is found
#   DotNetFramework_INCLUDE_DIRS   - Directories containing .NET Framework headers
#   DotNetFramework_LIBRARY_DIRS   - Directories containing .NET Framework libraries
#   DotNetFramework_LIBRARIES      - Libraries needed to link against .NET Framework
#

# Search for .NET Framework on Windows
if(WIN32)
    # Set default paths for .NET Framework

    set(DotNetFramework_ROOT_DIR "C:/Windows/Microsoft.NET/Framework")
    set(DotNetFramework_VERSION "v4.0.30319")
    set(DotNetFramework_INCLUDE_DIRS "${DotNetFramework_ROOT_DIR}/${DotNetFramework_VERSION}/include")
    set(DotNetFramework_LIBRARY_DIRS "${DotNetFramework_ROOT_DIR}/${DotNetFramework_VERSION}")

    # Find required .NET Framework libraries
    find_library(DotNetFramework_LIBRARIES
        NAMES "mscorlib.lib" "System.lib" "System.Data.lib" "System.Drawing.lib" "System.Windows.Forms.lib" "System.Xml.lib"
        HINTS ${DotNetFramework_LIBRARY_DIRS})

    # Check if .NET Framework is found
    if(DotNetFramework_LIBRARIES)
        set(DotNetFramework_FOUND TRUE)
    endif()

    # Print status message
    if(DotNetFramework_FOUND)
        message(STATUS "Found .NET Framework libraries: ${DotNetFramework_LIBRARIES}")
        message(STATUS "Found .NET Framework headers: ${DotNetFramework_INCLUDE_DIRS}")
    else()
        message(STATUS "Could not find .NET Framework")
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(DotNetFramework_FOUND DotNetFramework_INCLUDE_DIRS DotNetFramework_LIBRARY_DIRS DotNetFramework_LIBRARIES)
