/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2007 Stefan Westerfeld, 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __SFIDL_CLIENTC_H__
#define __SFIDL_CLIENTC_H__

#include <map>
#include <iostream>
#include <algorithm>
#include "sfidl-utils.hh"
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include "sfidl-cbase.hh"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

namespace Sfidl {
  
  class CodeGeneratorClientC : public CodeGeneratorCBase {
  protected:
    String prefix;

    Method methodWithObject (const Class& cd, const Method& md);
    void printProcedurePrototypes (PrefixSymbolMode mode);
    void printClassMacros();
    void printProcedureImpl ();
    void addBindingSpecificFiles (const String& binding_specific_files);
    
  public:
    CodeGeneratorClientC(const Parser& parser) : CodeGeneratorCBase (parser) {
    }
    void help ();
    bool run ();

    OptionVector getOptions();
    void setOption (const String& option, const String& value);
  };

};

#endif  /* __SFIDL_CLIENTC_H__ */

/* vim:set ts=8 sts=2 sw=2: */
