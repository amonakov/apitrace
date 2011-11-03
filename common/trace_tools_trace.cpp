/*********************************************************************
 *
 * Copyright 2011 Intel Corporation
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *********************************************************************/


#include <stdlib.h>

#include <iostream>

#include "os_path.hpp"
#include "trace_tools.hpp"



namespace trace {


#if defined(__APPLE__)
#define CLI_TRACE_VARIABLE "DYLD_LIBRARY_PATH"
#define CLI_TRACE_WRAPPER  "OpenGL"
#elif defined(_WIN32)
#define CLI_TRACE_VARIABLE ""
#define CLI_TRACE_WRAPPER  "opengl32.dll"
#else
#define CLI_TRACE_VARIABLE "LD_PRELOAD"
#define CLI_TRACE_WRAPPER  "glxtrace.so"
#endif


os::Path
findFile(const char *relPath,
         const char *absPath,
         bool verbose)
{
    os::Path complete;

    /* First look in the same directory from which this process is
     * running, (to support developers running a compiled program that
     * has not been installed. */
    os::Path process_dir = os::getProcessName();

    process_dir.trimFilename();

    complete = process_dir;
    complete.join(relPath);

    if (complete.exists())
        return complete;

    /* Second, look in the directory for installed wrappers. */
    complete = absPath;
    if (complete.exists())
        return complete;

    if (verbose) {
        std::cerr << "error: cannot find " << relPath << " or " << absPath << "\n";
    }

    return "";
}


int
traceProgram(char * const *argv,
             const char *output,
             bool verbose)
{
    os::Path wrapper;

    wrapper = findFile("wrappers/" CLI_TRACE_WRAPPER, APITRACE_WRAPPER_INSTALL_DIR CLI_TRACE_WRAPPER, verbose);

    if (!wrapper.length()) {
        return 1;
    }

#if defined(_WIN32)

    std::cerr <<
        "The 'apitrace trace' command is not supported for this operating system.\n"
        "Instead, you will need to copy opengl32.dll, d3d8.dll, or d3d9.dll from\n"
        APITRACE_WRAPPER_INSTALL_DIR "\n"
        "to the directory with the application to trace, then run the application.\n";

#else

#if defined(__APPLE__)
    /* On Mac OS X, using DYLD_LIBRARY_PATH, we actually set the
     * directory, not the file. */
    wrapper.trimFilename();
#endif

    if (verbose) {
        std::cerr << CLI_TRACE_VARIABLE << "=" << wrapper.str() << "\n";
    }

    /* FIXME: Don't modify the current environment */
    setenv(CLI_TRACE_VARIABLE, wrapper.str(), 1);

    if (output) {
        setenv("TRACE_FILE", output, 1);
    }

    if (verbose) {
        const char *sep = "";
        for (char * const * arg = argv; *arg; ++arg) {
            std::cerr << *arg << sep;
            sep = " ";
        }
        std::cerr << "\n";
    }

    execvp(argv[0], argv);

    unsetenv(CLI_TRACE_VARIABLE);
    if (output) {
        unsetenv("TRACE_FILE");
    }

    std::cerr << "error: Failed to execute " << argv[0] << "\n";
#endif

    return 1;
}


} /* namespace trace */