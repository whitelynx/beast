/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002 Stefan Westerfeld, 2003 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef _SFIDL_H__
#define _SFIDL_H__

#include <sfi/glib-extra.h>
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>
#include <string>
#include "sfidl-namespace.h"
#include "sfidl-options.h"
#include "sfidl-parser.h"

using namespace std;    // FIXME: stefan, kill using in header files

namespace Sfidl {
  
  class CodeGenerator {
  protected:
    const Parser& parser;
    const Options& options;
    
    vector<string> splitName (const string& name);
    string makeNamespaceSubst (const string& name);
    string makeLowerName (const string& name, char seperator = '_');
    string makeUpperName (const string& name);
    string makeMixedName (const string& name);
    string makeLMixedName (const string& name);
    
    CodeGenerator(const Parser& parser) : parser (parser), options (*Options::the()) {
    }
    
  public:
    virtual void run () = 0;
  };

  // FIXME: need to make C code generator public to get access to createTypeCode() and friends
  class CodeGeneratorC : public CodeGenerator {
  protected:
    
    void printInfoStrings (const string& name, const map<string,string>& infos);
    void printProcedure (const Method& mdef, bool proto = false, const string& className = "");
    
    bool choiceReverseSort(const ChoiceValue& e1, const ChoiceValue& e2);
    string makeGTypeName (const string& name);
  public:
    string makeParamSpec (const Param& pdef);
    string createTypeCode (const string& type, const string& name, int model);
    
  public:
    CodeGeneratorC(const Parser& parser) : CodeGenerator(parser) {
    }
    void run ();
  };
#define MODEL_ARG         0
#define MODEL_MEMBER      1
#define MODEL_RET         2
#define MODEL_ARRAY       3
#define MODEL_FREE        4
#define MODEL_COPY        5
#define MODEL_NEW         6
#define MODEL_FROM_VALUE  7
#define MODEL_TO_VALUE    8
#define MODEL_VCALL       9
#define MODEL_VCALL_ARG   10
#define MODEL_VCALL_CONV  11
#define MODEL_VCALL_CFREE 12
#define MODEL_VCALL_RET   13
#define MODEL_VCALL_RCONV 14
#define MODEL_VCALL_RFREE 15

  
};

#endif  /* _SFIDL_H__ */

/* vim:set ts=8 sts=2 sw=2: */
