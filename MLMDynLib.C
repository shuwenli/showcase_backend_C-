/* -*- c++ -*- */ /* vim:set ts=4 sts=4 sw=4 et syntax=cpp: */
/************************************************************************/
/*                                                                      */
/*  COPYRIGHT (c) 2006 Lucent Technologies                              */
/*  All Rights Reserved.                                                */
/*                                                                      */
/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF LUCENT TECHNOLOGIES  */
/*                                                                      */
/*  The copyright notice above does not evidence any                    */
/*  actual or intended publication of such source code.                 */
/*                                                                      */
/************************************************************************/

#include <exception>
#include <string>

#include <dlfcn.h>
#include <link.h>

#include "MLMDynLib.h"

/**
 * Constructor.
 *
 * Open an instance of the given library using the POSIX dlopen API.
 * @param lib identifies the name of the library to load.
 * @throws runtime_error if the library can't be loaded.
 */
MLMDynLib::MLMDynLib(const char* const lib) :
    m_dlHandle(dlopen(lib, RTLD_LAZY | RTLD_LOCAL))
{
    if (m_dlHandle == 0) // Can't open the library?
    {
        // Formulate an explanation and throw a run-time exception
        std::string reason("Library \"");
        reason += lib;
        reason += "\" can't be loaded.  Reason: ";
        const char * error = dlerror();
        if (error == 0)
        {
            error = "cannot be determined";
        }
        reason += error;
        throw std::runtime_error(reason);
    }
}

/**
 * Destructor.
 * Close the library opened in the constructor using the POSIX dlclose API.
 */
MLMDynLib::~MLMDynLib()
{
    if (m_dlHandle != 0) // Is the library open?
    {
        dlclose(m_dlHandle); // Close it
    }
    m_dlHandle = 0;
}

/**
 * Get the address of a symbol in the library using the POSIX dlsym API.
 *
 * @param name identifying the symbol in the library.
 * @return void* address of the given symbol or 0 if not found.
 */
void*
MLMDynLib::getAddress(const char* const name)
{
    if (m_dlHandle != 0) // Library was opened?
    {
        return dlsym(m_dlHandle, name); // Return the symbol address
    }

    return 0;
}
