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
#ifndef __SFIDL_GENERATOR_H__
#define __SFIDL_GENERATOR_H__

#include <map>
#include <iostream>
#include <algorithm>
#include "sfidl-utils.hh"
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

namespace Sfidl {
  
  class CodeGenerator {
  protected:
    const Parser& parser;
    const Options& options;

    bool generateHeader;
    bool generateSource;
    bool generateIdlLineNumbers;
    
    std::vector<String> splitName (const String& name);
    String makeLowerName (const String& name, char seperator = '_');
    String makeUpperName (const String& name);
    String makeMixedName (const String& name);
    String makeLMixedName (const String& name);

    enum WordCase {
      lower,
      Capitalized,
      semiCapitalized,
      UPPER
    };

    /*
     * translates a word into a given word case
     * i.e. toWordCase ("Hello", UPPER) == "HELLO"
     */
    String toWordCase (const String& word, WordCase wc);
    String joinName (const std::vector<String>& name, const String& seperator, WordCase wc);

    enum NamespaceType {
      NONE,     /* no namespace */
      ABSOLUTE, /* in C++ means :: prefix */
      /* RELATIVE indicated by NamespaceHelper instead */
    };

    /*
     * rename is a function for renaming types according to rules
     * 
     * name: the name to be renamed
     * namespace_wc: the desired case for the namespace
     * namespace_join: how to join the namespace - note that if namespace_join
     *   is "::", then "::" will also be prefixed to the result, whereas if
     *   namespace_join is "_", it will only be used to seperate the namespaces
     *   (this is required/useful for C++)
     * namespace_append: words to append to the namespace
     * typename_wc: the desired case for the typename
     * typename_join: how to join the typename
     */
    String
    rename (NamespaceType namespace_type, const String& name, WordCase namespace_wc,
	    const String &namespace_join, const std::vector<String> &namespace_append,
	    WordCase typename_wc, const String &typename_join);

    /*
     * rename is a function for renaming types according to rules
     * 
     * nsh: namespace helper indicates which namespace to be relative to
     * namespace_wc: the desired case for the namespace
     * namespace_join: how to join the namespace
     * namespace_append: words to append to the namespace
     * typename_wc: the desired case for the typename
     * typename_join: how to join the typename
     */
    String
    rename (NamespaceHelper& nsh, const String& name, WordCase namespace_wc,
	    const String& namespace_join, const std::vector<String>& namespace_append,
	    WordCase typename_wc, const String& typename_join);
    
    CodeGenerator(const Parser& parser)
      : parser (parser), options (*Options::the()),
        generateHeader (true), generateSource (false),
	generateIdlLineNumbers (true)
    {
    }
   
  public:
    /*
     * returns the options supported by this code generator (used by the option parser)
     * the first element of the pair is the option (i.e. "--source")
     * the second element of the pair is true when the option should be followed by a
     *   user argument (as in "--prefix beast"), false otherwise (i.e. "--source")
     */
    virtual OptionVector getOptions();

    /*
     * called by the option parser when an option is set
     * option is the option (i.e. "--prefix")
     * value is the value (i.e. "beast"), and "1" for options without user argument
     */
    virtual void setOption (const String& option, const String& value);

    /*
     * prints help for the options supported by this code generator
     */
    virtual void help();

    /*
     * run generates the code, and should return true if successful, false otherwise
     * (for instance if inconsistent options were given to the code generator)
     */
    virtual bool run () = 0;
    virtual ~CodeGenerator() {
    }
  };

};

#endif  /* __SFIDL_GENERATOR_H__ */

/* vim:set ts=8 sts=2 sw=2: */
