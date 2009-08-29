/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2003 Tim Janik
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
#include "sfidl-namespace.hh"
#include "sfidl-factory.hh"
#include "sfidl-utils.hh"
#include "sfidl-generator.hh"

#include <string.h>
#include <stdio.h>
#include <list>
#include <map>

namespace {
using namespace Sfidl;
using std::make_pair;

static const gchar*
canonify_name (const String& s,
               const char    replace = '-')
{
  /* canonify type names which contain e.g. underscores (procedures) */
  gchar *tmp = g_strcanon (g_strdup (s.c_str()),
                           G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "+",
                           replace);
  String d = tmp;
  g_free (tmp);
  return g_intern_string (d.c_str());
}

static const gchar*
UPPER_CASE (const String &s)
{
  String d = s;
  for (guint i = 0; i < d.size(); i++)
    if (d[i] >= 'a' && d[i] <= 'z')
      d[i] += 'A' - 'a';
    else if (d[i] >= 'A' && d[i] <= 'Z')
      ;
    else if (d[i] >= '0' && d[i] <= '9')
      ;
    else
      d[i] = '_';
  return g_intern_string (d.c_str());
}

static const char*
intern_escape (const String &s)
{
  char *x = g_strescape (s.c_str(), 0);
  String e = String ("\"") + x + "\"";
  g_free (x);
  const char *result = g_intern_string (e.c_str());
  return result;
}

static String
include_relative (String path,
                  String source_file)
{
  if (g_path_is_absolute (path.c_str()))
    return path;
  gchar *dir = g_path_get_dirname (source_file.c_str());
  String apath = String(dir) + G_DIR_SEPARATOR_S + path;
  g_free (dir);
  return apath;
}

static String
glue_untyped_pspec_constructor (const Parser &parser,
                                const Param  &param)
{
  const String group = (param.group != "") ? param.group.escaped() : "NULL";
  const String file_line_args = ",\"" + param.file + "\"," + string_from_int (param.line);
  switch (parser.typeOf (param.type))
    {
    case OBJECT:
      {
        String pspec = "sfidl_pspec_Object";
        if (param.args == "")
          pspec += "_default (" + group + file_line_args + ",\"" + param.name + "\")";
        else
          pspec += " (" + group + file_line_args + ",\"" + param.name + "\"," + param.args + ")";
        return pspec;
      }
    default:
      {
        String pspec = "sfidl_pspec_" + param.pspec;
        if (param.args == "")
          pspec += "_default (" + group + file_line_args + ",\"" + param.name + "\")";
        else
          pspec += " (" + group + file_line_args + ",\"" + param.name + "\"," + param.args + ")";
        return pspec;
      }
    }
}

class LanguageBindingCoreCxx : public CodeGenerator {
  struct Image {
    String file;
    String method;
    Image (const String &f = "",
           const String &m = "")
      : file (f), method (m)
    {}
  };
  vector<Image> images;
  vector<const Method*> procs;
  vector<String> alltypes;
  void
  push_type (const String &kind,
             const String &type_name)
  {
    String s;
    s += kind + " (" + type_name + ")";
    alltypes.push_back (s);
  }
  OptionVector
  getOptions()
  {
    OptionVector opts = CodeGenerator::getOptions();    // FIXME: adopt parent type
    opts.push_back (make_pair ("--macro", true));
    return opts;
  }
  String alltypes_macro;
  void
  setOption (const String& option,
             const String& value)
  {
    if (option == "--macro")
      {
        alltypes_macro = value;
      }
    else
      {
        CodeGenerator::setOption (option, value);       // FIXME: adopt parent type
      }
  }
public:
  LanguageBindingCoreCxx (const Parser &p)
    : CodeGenerator (p)
  {
  }
  const char*
  intern (const String &str)
  {
    return g_intern_string (str.c_str());
  }
  String
  untyped_pspec_constructor (const Param &param)
  {
    const String group = (param.group != "") ? param.group.escaped() : "NULL";
    const String file_line_args = ",\"" + param.file + "\"," + string_from_int (param.line);
    switch (parser.typeOf (param.type))
      {
      case CHOICE:
        {
          String pspec = "sfidl_pspec_Choice";
          if (param.args == "")
            pspec += "_default";
          pspec += " (" + group + file_line_args + ", \"" + param.name + "\", ";
          if (param.args != "")
            pspec += param.args + ", ";
          pspec += param.type + "_choice_values()";
          pspec += ")";
          return pspec;
        }
      case SFIREC:
        {
          String pspec = "sfidl_pspec_Rec";
          if (param.args == "")
            pspec += "_default (" + group + file_line_args + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + file_line_args + ", \"" + param.name + "\", " + param.args;
          pspec += ")";
          return pspec;
        }
      case RECORD:
        {
          String pspec = "sfidl_pspec_Record";
          if (param.args == "")
            pspec += "_default (" + group + file_line_args + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + file_line_args + ", \"" + param.name + "\", " + param.args + ", ";
          pspec += param.type + "::get_fields()";
          pspec += ")";
          return pspec;
        }
      case SEQUENCE:
        {
          String pspec = "sfidl_pspec_Sequence";
          if (param.args == "")
            pspec += "_default (" + group + file_line_args + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + file_line_args + ", \"" + param.name + "\", " + param.args + ", ";
          pspec += param.type + "::get_element()";
          pspec += ")";
          return pspec;
        }
      default:  return glue_untyped_pspec_constructor (parser, param);
      }
  }
  const char*
  make_TYPE_NAME (const String &type_name)
  {
    vector<String> astrs;
    astrs.push_back ("TYPE");
    return intern (rename (ABSOLUTE, type_name, UPPER, "_", astrs, UPPER, "_").c_str());
  }
  const char*
  make_IS_NAME (const String &type_name)
  {
    vector<String> astrs;
    astrs.push_back ("IS");
    return intern (rename (ABSOLUTE, type_name, UPPER, "_", astrs, UPPER, "_").c_str());
  }
  const char*
  make_fqtn (const String &type_name,
             const String &append = "")
  {
    vector<String> empty;
    String s = rename (ABSOLUTE, type_name, Capitalized, "::", empty, Capitalized, "");
    s += append;
    return intern (s);
  }
  const char*
  make_PrefixedTypeName (const String &type_name,
                         const String &append = "")
  {
    vector<String> empty;
    String s = rename (ABSOLUTE, type_name, Capitalized, "", empty, Capitalized, "");
    s += append;
    return intern (s);
  }
  const char*
  pure_TypeName (const String &type_name)
  {
    vector<String> empty;
    String s = rename (NONE, type_name, UPPER, "", empty, Capitalized, "");
    return intern (s);
  }
  const char*
  make_FULL_UPPER (const String &type_name)
  {
    vector<String> empty;
    String s = rename (ABSOLUTE, type_name, UPPER, "_", empty, UPPER, "_");
    return intern (s);
  }
  const char*
  pure_UPPER (const String &type_name)
  {
    vector<String> empty;
    String s = rename (NONE, type_name, UPPER, "_", empty, UPPER, "_");
    return intern (s);
  }
  const char*
  make_scheme_name (const String &type_name)
  {
    vector<String> empty;
    return intern (rename (ABSOLUTE, type_name, lower, "-", empty, lower, "-").c_str());
  }
  const char*
  make_full_lower (const String &type_name)
  {
    vector<String> empty;
    return intern (rename (ABSOLUTE, type_name, lower, "_", empty, lower, "_").c_str());
  }
  const char*
  pure_lower (const String &type_name)
  {
    vector<String> empty;
    return intern (rename (NONE, type_name, lower, "_", empty, lower, "_").c_str());
  }
  const Class*
  find_class (const String &type_name)
  {
    for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
      if (ci->name == type_name)
        return &*ci;
    return NULL;
  }
  bool
  is_cxx_class (const String &type_name)
  {
    const String cxxbase = "Bse::CxxBase";      // FIXME: hardcoding Bse C++ base type
    const Class *cc = find_class (type_name);
    while (cc)
      {
        if (cc->name == cxxbase)
          return true;
        String xxx = cc->inherits;
        cc = find_class (cc->inherits);
      }
    return false;
  }
  String
  typed_pspec_constructor (const Param &param)
  {
    const String group = (param.group != "") ? param.group.escaped() : "NULL";
    const String file_line_args = ",\"" + param.file + "\"," + string_from_int (param.line);
    switch (parser.typeOf (param.type))
      {
      case CHOICE:
        {
          String pspec = "sfidl_pspec_GEnum";
          if (param.args == "")
            pspec += "_default";
          pspec += " (" + group + file_line_args + ", \"" + param.name + "\", ";
          if (param.args != "")
            pspec += param.args + ", ";
          pspec += make_TYPE_NAME (param.type);
          pspec += ")";
          return pspec;
        }
      case RECORD:
        {
          String pspec = "sfidl_pspec_BoxedRec";
          if (param.args == "")
            pspec += "_default (" + group + file_line_args + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + file_line_args + ", \"" + param.name + "\", " + param.args + ", ";
          pspec += make_TYPE_NAME (param.type);
          pspec += ")";
          return pspec;
        }
      case SEQUENCE:
        {
          String pspec = "sfidl_pspec_BoxedSeq";
          if (param.args == "")
            pspec += "_default (" + group + file_line_args + ", \"" + param.name + "\", ";
          else
            pspec += " (" + group + file_line_args + ", \"" + param.name + "\", " + param.args + ", ";
          pspec += make_TYPE_NAME (param.type);
          pspec += ")";
          return pspec;
        }
      case OBJECT:
        {
          String pspec = "sfidl_pspec_TypedObject";
          if (param.args == "")
            pspec += "_default";
          pspec += " (" + group + file_line_args + ", \"" + param.name + "\", ";
          if (param.args != "")
            pspec += param.args + ", ";
          pspec += make_TYPE_NAME (param.type);
          pspec += ")";
          return pspec;
        }
      default:    return untyped_pspec_constructor (param);
      }
  }
  const char*
  TypeField (const String& type)
  {
    switch (parser.typeOf (type))
      {
      case VOID:        return "void";
      case BOOL:        return "bool";
      case INT:         return "Sfi::Int";
      case NUM:         return "Sfi::Num";
      case REAL:        return "Sfi::Real";
      case CHOICE:      return make_fqtn (type);
      case STRING:      return "Sfi::String";
      case BBLOCK:      return "Sfi::BBlock";
      case FBLOCK:      return "Sfi::FBlock";
      case SFIREC:      return "Sfi::Rec";
      case RECORD:      return make_fqtn (type, "Handle");
      case SEQUENCE:    return make_fqtn (type);
      case OBJECT:      return is_cxx_class (type) ? make_fqtn (type, "*") : make_PrefixedTypeName (type, "*");
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  TypeArg (const String &type)
  {
    switch (parser.typeOf (type))
      {
      case VOID:
      case BOOL:
      case INT:
      case NUM:
      case REAL:
      case CHOICE:      return TypeField (type);
      case STRING:
      case BBLOCK:
      case FBLOCK:
      case SFIREC:
      case RECORD:
      case SEQUENCE:    return intern (String ("const ") + TypeField (type) + " &");
      case OBJECT:      return TypeField (type);
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  TypeRet (const String& type)
  {
    switch (parser.typeOf (type))
      {
      default:          return TypeField (type);
      }
  }
  const char*
  func_value_set_param (const String type)
  {
    String s;
    switch (parser.typeOf (type))
      {
      case BOOL:        return "sfi_value_set_bool";
      case INT:         return "sfi_value_set_int";
      case NUM:         return "sfi_value_set_num";
      case REAL:        return "sfi_value_set_real";
      case CHOICE:      return intern (s + "sfi_value_set_enum_auto " +
                                       "SFI_START_ARGS() " + make_TYPE_NAME (type) + ", SFI_END_ARGS2");
      case STRING:      return "::Sfi::String::value_set_string";
      case BBLOCK:      return "::Sfi::BBlock::value_set_bblock";
      case FBLOCK:      return "::Sfi::FBlock::value_set_fblock";
      case SFIREC:      return "::Sfi::Rec::value_set_rec";
      case RECORD:
      case SEQUENCE:    return intern (make_fqtn (type) + String ("::value_set_boxed"));
      case OBJECT:
        if (is_cxx_class (type))
          return intern (String() + "::Bse::CxxBase::value_set_casted< " + type + ", " + type + "Base>");
        else
          return intern (String() + "::Bse::CxxBase::value_set_gobject");
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  func_value_get_param (const String type)
  {
    String s;
    switch (parser.typeOf (type))
      {
      case BOOL:        return "sfi_value_get_bool";
      case INT:         return "sfi_value_get_int";
      case NUM:         return "sfi_value_get_num";
      case REAL:        return "sfi_value_get_real";
      case CHOICE:      return intern (s + "(" + make_fqtn (type) + ") sfi_value_get_enum_auto " +
                                       "SFI_START_ARGS() " + make_TYPE_NAME (type) + ", SFI_END_ARGS1");
      case STRING:      return "::Sfi::String::value_get_string";
      case BBLOCK:      return "::Sfi::BBlock::value_get_bblock";
      case FBLOCK:      return "::Sfi::FBlock::value_get_fblock";
      case SFIREC:      return "::Sfi::Rec::value_get_rec";
      case RECORD:
      case SEQUENCE:    return intern (make_fqtn (type) + String ("::value_get_boxed"));
      case OBJECT:
        if (is_cxx_class (type))
          return intern (String ("(") + make_fqtn (type) + "*) " +
                         "::Bse::CxxBase::value_get_object< " + make_fqtn (type) + "Base* >");
        else
          return intern (String ("::Bse::CxxBase::value_get_gobject< ") + make_PrefixedTypeName (type) + ">");
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  make_SFI_TYPE_NAME (const String &type)
  {
    switch (parser.typeOf (type))
      {
      case CHOICE:      return "SFI_TYPE_CHOICE";
      case RECORD:      return "SFI_TYPE_REC";
      case SEQUENCE:    return "SFI_TYPE_SEQ";
      case OBJECT:      return "SFI_TYPE_PROXY";
      default:          return make_TYPE_NAME (type);
      }
  }
  void
  generate_choice_prototypes (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* choice prototypes */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        const char *name = nspace.printable_form (ci->name);
        g_print ("static SfiChoiceValues %s_choice_values();\n", name);
      }
  }
  void
  generate_choice_implementations (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* choice implementations */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        const char *name = nspace.printable_form (ci->name);
        g_print ("static SfiChoiceValues\n");
        g_print ("%s_choice_values()\n", name);
        g_print ("{\n");
        g_print ("  static SfiChoiceValue values[%zu];\n", ci->contents.size());
        g_print ("  static const SfiChoiceValues choice_values = {\n");
        g_print ("    G_N_ELEMENTS (values), values,\n");
        g_print ("  };\n");
        g_print ("  if (!values[0].choice_ident)\n    {\n");
        int i = 0;
        for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); i++, vi++)
          {
            g_print ("      values[%u].choice_ident = \"%s\";\n", i, make_FULL_UPPER (vi->name));
            g_print ("      values[%u].choice_label = %s;\n", i, vi->label.escaped().c_str());
            g_print ("      values[%u].choice_blurb = %s;\n", i, vi->blurb.escaped().c_str());
          }
        g_print ("  }\n");
        g_print ("  return choice_values;\n");
        g_print ("}\n\n");
      }
  }
  void
  generate_enum_prototypes (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* enum prototypes */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        // const char *name = nspace.printable_form (ci->name);
        g_print ("#define %s\t\tBSE_CXX_DECLARED_ENUM_TYPE (%s, %s)\n",
                make_TYPE_NAME (ci->name),
                nspace.namespaceOf (ci->name).c_str(),
                pure_TypeName (ci->name));
        push_type ("ENUM", pure_TypeName (ci->name));
      }
  }
  void
  generate_enum_definitions (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* enum definitions */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        const char *name = nspace.printable_form (ci->name);
        g_print ("enum %s {\n", name);
        for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
          g_print ("  %s = %d,\n", pure_UPPER (vi->name), vi->value);
        g_print ("};\n");
      }
  }
  void
  generate_enum_declarations (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* enum declarations */\n");
    for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        // const char *name = nspace.printable_form (ci->name);
        g_print ("BSE_CXX_DECLARE_ENUM (%s, \"%s\", %zu,\n",
                pure_TypeName (ci->name), make_PrefixedTypeName (ci->name), ci->contents.size());
        for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
          g_print ("  *v++ = ::Bse::EnumValue (%s, \"%s\", %s );\n",
                  pure_UPPER (vi->name), make_FULL_UPPER (vi->name), vi->label.escaped().c_str());
        g_print (");\n");
      }
  }
  void
  generate_record_prototypes (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* record prototypes */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        
        nspace.setFromSymbol(ri->name);
        const char *name = nspace.printable_form (ri->name);
        
        g_print ("class %s;\n", pure_TypeName (ri->name));
        g_print ("typedef Sfi::RecordHandle<%s> %sHandle;\n", name, name);
        g_print ("#define %s\t\tBSE_CXX_DECLARED_RECORD_TYPE (%s, %s)\n",
                make_TYPE_NAME (ri->name),
                nspace.namespaceOf (ri->name).c_str(),
                pure_TypeName (ri->name));
        push_type ("RECORD", pure_TypeName (ri->name));
      }
  }
  void
  generate_record_definitions (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* record definitions */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        nspace.setFromSymbol(ri->name);
        
        g_print ("class %s : public ::Sfi::GNewable {\n", pure_TypeName (ri->name));
        g_print ("public:\n");
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            g_print ("  %s %s;\n", TypeField (pi->type), pi->name.c_str());
          }
        g_print ("  static inline %s from_rec (SfiRec *rec);\n", TypeRet (ri->name));
        g_print ("  static inline SfiRec *to_rec (%s ptr);\n", TypeArg (ri->name));
        g_print ("  static inline %s value_get_boxed (const GValue *value);\n", TypeRet (ri->name));
        g_print ("  static inline void value_set_boxed (GValue *value, %s self);\n", TypeArg (ri->name));
        g_print ("  static inline const char* options   () { return %s; }\n", ri->infos.get("options").escaped().c_str());
        g_print ("  static inline const char* blurb     () { return %s; }\n", ri->infos.get("blurb").escaped().c_str());
        g_print ("  static inline const char* authors   () { return %s; }\n", ri->infos.get("authors").escaped().c_str());
        g_print ("  static inline const char* license   () { return %s; }\n", ri->infos.get("license").escaped().c_str());
        g_print ("  static inline const char* type_name () { return \"%s\"; }\n", make_PrefixedTypeName (ri->name));
        g_print ("  static inline SfiRecFields get_fields ();\n");
        g_print ("};\n");
        g_print ("\n");
      }
  }
  void
  generate_record_declarations (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* record type declarations */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        nspace.setFromSymbol(ri->name);
        
        g_print ("BSE_CXX_DECLARE_RECORD (%s);\n", pure_TypeName (ri->name));
        g_print ("\n");
      }
  }
  void
  generate_record_implementations (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* record implementations */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        nspace.setFromSymbol(ri->name);
        const char *nname = nspace.printable_form (ri->name);
        
        g_print ("%s\n", TypeRet (ri->name));
        g_print ("%s::from_rec (SfiRec *sfi_rec)\n", nname);
        g_print ("{\n");
        g_print ("  GValue *element;\n");
        g_print ("\n");
        g_print ("  if (!sfi_rec)\n");
        g_print ("    return Sfi::INIT_NULL;\n");
        g_print ("\n");
        g_print ("  %s rec = Sfi::INIT_DEFAULT;\n", TypeField (ri->name));
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            g_print ("  element = sfi_rec_get (sfi_rec, \"%s\");\n", pi->name.c_str());
            g_print ("  if (element)\n");
            g_print ("    rec->%s = %s (element);\n", pi->name.c_str(), func_value_get_param (pi->type));
          }
        g_print ("  return rec;\n");
        g_print ("}\n\n");
        
        g_print ("SfiRec *\n");
        g_print ("%s::to_rec (%s rec)\n", nname, TypeArg (ri->name));
        g_print ("{\n");
        g_print ("  SfiRec *sfi_rec;\n");
        g_print ("  GValue *element;\n");
        g_print ("\n");
        g_print ("  if (!rec)\n");
        g_print ("    return NULL;\n");
        g_print ("\n");
        g_print ("  sfi_rec = sfi_rec_new ();\n");
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            g_print ("  element = sfi_rec_forced_get (sfi_rec, \"%s\", %s);\n",
                    pi->name.c_str(), make_SFI_TYPE_NAME (pi->type));
            g_print ("  %s (element, rec->%s);\n", func_value_set_param (pi->type), pi->name.c_str());
          }
        g_print ("  return sfi_rec;\n");
        g_print ("}\n\n");
        
        g_print ("%s\n", TypeRet (ri->name));
        g_print ("%s::value_get_boxed (const GValue *value)\n", nname);
        g_print ("{\n");
        g_print ("  return %s::value_get_boxed (value);\n", TypeRet (ri->name));
        g_print ("}\n\n");
        g_print ("void\n");
        g_print ("%s::value_set_boxed (GValue *value, %s self)\n", nname, TypeArg (ri->name));
        g_print ("{\n");
        g_print ("  %s::value_set_boxed (value, self);\n", TypeRet (ri->name));
        g_print ("}\n\n");
        
        g_print ("SfiRecFields\n");
        g_print ("%s::get_fields()\n", nname);
        g_print ("{\n");
        g_print ("  static SfiRecFields rfields = { 0, NULL };\n");
        g_print ("  if (!rfields.n_fields)\n");
        g_print ("    {\n");
        g_print ("      static GParamSpec *fields[%zu + 1];\n", ri->contents.size());
        g_print ("      rfields.n_fields = %zu;\n", ri->contents.size());
        guint j = 0;
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            // g_print ("#line %u \"%s\"\n", pi->line, parser.fileName().c_str());
            g_print ("      fields[%u] = %s;\n", j++, untyped_pspec_constructor (*pi).c_str());
          }
        g_print ("      rfields.fields = fields;\n");
        g_print ("    }\n");
        g_print ("  return rfields;\n");
        g_print ("}\n");
      }
  }
  void
  generate_sequence_prototypes (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* sequence prototypes */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        nspace.setFromSymbol(si->name);
        
        g_print ("class %s;\n", pure_TypeName (si->name));
        g_print ("#define %s\t\tBSE_CXX_DECLARED_SEQUENCE_TYPE (%s, %s)\n",
                make_TYPE_NAME (si->name),
                nspace.namespaceOf (si->name).c_str(),
                pure_TypeName (si->name));
        push_type ("SEQUENCE", pure_TypeName (si->name));
      }
  }
  void
  generate_sequence_definitions (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* sequence definitions */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        nspace.setFromSymbol(si->name);
        
        g_print ("class %s : public Sfi::Sequence< %s > {\n", pure_TypeName (si->name), TypeField (si->content.type));
        g_print ("public:\n");
        g_print ("  %s (unsigned int n = 0) : Sfi::Sequence< %s > (n) {}\n", pure_TypeName (si->name), TypeField (si->content.type));
        g_print ("  static inline %s from_seq (SfiSeq *seq);\n", TypeRet (si->name));
        g_print ("  static inline SfiSeq *to_seq (%s seq);\n", TypeArg (si->name));
        g_print ("  static inline %s value_get_boxed (const GValue *value);\n", TypeRet (si->name));
        g_print ("  static inline void value_set_boxed (GValue *value, %s self);\n", TypeArg (si->name));
        g_print ("  static inline const char* options   () { return %s; }\n", si->infos.get("options").escaped().c_str());
        g_print ("  static inline const char* blurb     () { return %s; }\n", si->infos.get("blurb").escaped().c_str());
        g_print ("  static inline const char* authors   () { return %s; }\n", si->infos.get("authors").escaped().c_str());
        g_print ("  static inline const char* license   () { return %s; }\n", si->infos.get("license").escaped().c_str());
        g_print ("  static inline const char* type_name () { return \"%s\"; }\n", make_PrefixedTypeName (si->name));
        g_print ("  static inline GParamSpec* get_element ();\n");
        g_print ("};\n");
        g_print ("\n");
      }
  }
  void
  generate_sequence_declarations (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* sequence type declarations */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        nspace.setFromSymbol(si->name);

        g_print ("BSE_CXX_DECLARE_SEQUENCE (%s);\n", pure_TypeName (si->name));
        g_print ("\n");
      }
  }
  void
  generate_sequence_implementations (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* sequence implementations */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        nspace.setFromSymbol(si->name);
        const char *nname = nspace.printable_form (si->name);
        
        g_print ("%s\n", TypeRet (si->name));
        g_print ("%s::from_seq (SfiSeq *sfi_seq)\n", nname);
        g_print ("{\n");
        g_print ("  %s cseq;\n", TypeRet (si->name));
        g_print ("  guint i, length;\n");
        g_print ("\n");
        g_print ("  if (!sfi_seq)\n");
        g_print ("    return cseq;\n");
        g_print ("\n");
        g_print ("  length = sfi_seq_length (sfi_seq);\n");
        g_print ("  cseq.resize (length);\n");
        g_print ("  for (i = 0; i < length; i++)\n");
        g_print ("    {\n");
        g_print ("      GValue *element = sfi_seq_get (sfi_seq, i);\n");
        g_print ("      cseq[i] = %s (element);\n", func_value_get_param (si->content.type));
        g_print ("    }\n");
        g_print ("  return cseq;\n");
        g_print ("}\n\n");
        
        g_print ("SfiSeq *\n");
        g_print ("%s::to_seq (%s cseq)\n", nname, TypeArg (si->name));
        g_print ("{\n");
        g_print ("  SfiSeq *sfi_seq = sfi_seq_new ();\n");
        g_print ("  for (guint i = 0; i < cseq.length(); i++)\n");
        g_print ("    {\n");
        g_print ("      GValue *element = sfi_seq_append_empty (sfi_seq, %s);\n", make_SFI_TYPE_NAME (si->content.type));
        g_print ("      %s (element, cseq[i]);\n", func_value_set_param (si->content.type));
        g_print ("    }\n");
        g_print ("  return sfi_seq;\n");
        g_print ("}\n\n");
        
        g_print ("%s\n", TypeRet (si->name));
        g_print ("%s::value_get_boxed (const GValue *value)\n", nname);
        g_print ("{\n");
        g_print ("  return ::Sfi::cxx_value_get_boxed_sequence< %s> (value);\n", nname);
        g_print ("}\n\n");
        g_print ("void\n");
        g_print ("%s::value_set_boxed (GValue *value, %s self)\n", nname, TypeArg (si->name));
        g_print ("{\n");
        g_print ("  ::Sfi::cxx_value_set_boxed_sequence< %s> (value, self);\n", nname);
        g_print ("}\n\n");
        
        g_print ("GParamSpec*\n");
        g_print ("%s::get_element()\n", nname);
        g_print ("{\n");
        g_print ("  static GParamSpec *element = NULL;\n");
        g_print ("  if (!element)\n");
        // g_print ("#line %u \"%s\"\n", si->content.line, parser.fileName().c_str());
        g_print ("    element = %s;\n", untyped_pspec_constructor (si->content).c_str());
        g_print ("  return element;\n");
        g_print ("}\n\n");
      }
  }
  void
  generate_class_prototypes (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* class prototypes */\n");
    for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        g_print ("class %sBase;\n", pure_TypeName (ci->name));
        g_print ("class %s;\n", pure_TypeName (ci->name));
        g_print ("#define %s\t\tBSE_CXX_DECLARED_CLASS_TYPE (%s, %s)\n",
                make_TYPE_NAME (ci->name),
                nspace.namespaceOf (ci->name).c_str(),
                pure_TypeName (ci->name));
        g_print ("#define %s(o) (::Bse::CxxBase::instance_is_a (o, %s))\n",
                make_IS_NAME (ci->name), make_TYPE_NAME (ci->name));
        push_type ("EFFECT", pure_TypeName (ci->name));
      }
  }
  bool
  class_has_automation_properties (const Class &klass)
  {
    /* figure whether automation properties are required */
    for (vector<Param>::const_iterator pi = klass.properties.begin(); pi != klass.properties.end(); pi++)
      if (g_option_check (pi->literal_options.c_str(), "automate"))
        return true;
    return false;
  }
  void
  generate_class_definitions (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* class definitions */\n");
    for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        // const char *name = nspace.printable_form (ci->name);
        const char *ctName = pure_TypeName (ci->name);
        const char *ctNameBase = intern (ctName + String ("Base"));
        const char *ctProperties = intern (ctName + String ("Properties"));
        const char *ctPropertyID = intern (ctName + String ("PropertyID"));
        vector<String> destroy_jobs;
        
        /* skeleton class declaration + type macro */
        g_print ("BSE_CXX_DECLARE_CLASS (%s);\n", pure_TypeName (ci->name));
        g_print ("class %s : public %s {\n", ctNameBase, make_fqtn (ci->inherits));
        
        /* class Info strings */
        /* pixstream(), this is a bit of a hack, we make it a template rather than
         * a normal inline method to avoid huge images in debugging code
         */
        String icon = ci->infos.get("icon");
        String pstream = "NULL";
        if (icon != "")
          {
            g_print ("  template<bool> static inline const unsigned char* inlined_pixstream();\n");
            images.push_back (Image (include_relative (icon, ci->file),
                                     String ("template<bool> const unsigned char*\n") +
                                     make_fqtn (ci->name) + "Base" +
                                     "::inlined_pixstream()"));
            pstream = "inlined_pixstream<true>()";
          }
        g_print ("public:\n");
        g_print ("  static inline const unsigned char* pixstream () { return %s; }\n", pstream.c_str());
        g_print ("  static void               class_init (::Bse::CxxBaseClass *klass);\n");
        g_print ("  static inline const char* options   () { return %s; }\n", ci->infos.get("options").escaped().c_str());
        g_print ("  static inline const char* category  () { static const char *c = NULL;\n");
        g_print ("    return c ? c : c = sfi_category_concat (\"/Modules\", %s); }\n",
                ci->infos.get("category").escaped(" ").c_str()); // untranslated
        g_print ("  static inline const char* i18n_category  () { static const char *c = NULL;\n");
        g_print ("    return c ? c : c = sfi_category_concat (\"/Modules\", %s); }\n",
                ci->infos.get("category").escaped().c_str());    // translated
        g_print ("  static inline const char* blurb     () { return %s; }\n", ci->infos.get("blurb").escaped().c_str());
        g_print ("  static inline const char* authors   () { return %s; }\n", ci->infos.get("authors").escaped().c_str());
        g_print ("  static inline const char* license   () { return %s; }\n", ci->infos.get("license").escaped().c_str());
        g_print ("  static inline const char* type_name () { return \"%s\"; }\n", make_PrefixedTypeName (ci->name));
        
        /* i/j/o channel names */
        if (ci->istreams.size())
          {
            g_print ("public:\n");
            g_print ("  enum {\n");
            for (vector<Stream>::const_iterator si = ci->istreams.begin(); si != ci->istreams.end(); si++)
              g_print ("    ICHANNEL_%s,\n", pure_UPPER (si->ident));
            g_print ("    N_ICHANNELS\n  };\n");
          }
        if (ci->jstreams.size())
          {
            g_print ("public:\n");
            g_print ("  enum {\n");
            for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
              g_print ("    JCHANNEL_%s,\n", pure_UPPER (si->ident));
            g_print ("    N_JCHANNELS\n  };\n");
          }
        if (ci->ostreams.size())
          {
            g_print ("public:\n");
            g_print ("  enum {\n");
            for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
              g_print ("    OCHANNEL_%s,\n", pure_UPPER (si->ident));
            g_print ("    N_OCHANNELS\n  };\n");
          }
        
        /* property IDs */
        g_print ("protected:\n  enum %s {\n", ctPropertyID);
        if (ci->properties.begin() != ci->properties.end())
          {
            vector<Param>::const_iterator pi = ci->properties.begin();
            g_print ("    PROP_%s = 1,\n", pure_UPPER (pi->name));
            for (pi++; pi != ci->properties.end(); pi++)
              g_print ("    PROP_%s,\n", pure_UPPER (pi->name));
          }
        g_print ("  };\n");
        
        /* "Properties" structure for synthesis modules */
        if (ci->istreams.size() + ci->jstreams.size() + ci->ostreams.size())
          {
            g_print ("public:\n");
            g_print ("  /* \"transport\" structure to configure synthesis modules from properties */\n");
            g_print ("  struct %s {\n", ctProperties);
            g_print ("    typedef %s IDType;\n", ctPropertyID);
            for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
              g_print ("    %s %s;\n", TypeField (pi->type), pi->name.c_str());
            g_print ("    explicit %s (%s *p) ", ctProperties, ctNameBase);
            for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
              g_print ("%c\n      %s (p->%s)", pi == ci->properties.begin() ? ':' : ',', pi->name.c_str(), pi->name.c_str());
            g_print ("\n    {\n");
            g_print ("    }\n");
            g_print ("  };\n");
          }
        
        /* auto-update type */
        g_print ("protected:\n");
        g_print ("  typedef %s AutoUpdateCategory;\n", class_has_automation_properties (*ci) ? "::Bse::SynthesisModule::NeedAutoUpdateTag" : "void");

        /* property fields */
        g_print ("protected:\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          {
            g_print ("  %s %s;\n", TypeField (pi->type), pi->name.c_str());
            if (g_option_check (pi->literal_options.c_str(), "automate"))
              g_print ("  guint64 last__%s;\n", pi->name.c_str());
          }
        
        /* get_property() */
        g_print ("public:\n");
        g_print ("  void get_property (%s prop_id, ::Bse::Value &value, GParamSpec *pspec)\n", ctPropertyID);
        g_print ("  {\n");
        g_print ("    switch (prop_id) {\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          {
            g_print ("    case PROP_%s:\n", pure_UPPER (pi->name));
            g_print ("      %s (&value, %s);\n", func_value_set_param (pi->type), pi->name.c_str());
            g_print ("    break;\n");
          }
        g_print ("    };\n");
        g_print ("  }\n");
        
        /* set_property() */
        g_print ("  void set_property (%s prop_id, const ::Bse::Value &value, GParamSpec *pspec)\n", ctPropertyID);
        g_print ("  {\n");
        g_print ("    switch (prop_id) {\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          {
            g_print ("    case PROP_%s:\n", pure_UPPER (pi->name));
            g_print ("      %s = %s (&value);\n", pi->name.c_str(), func_value_get_param (pi->type));
            g_print ("    break;\n");
          }
        g_print ("    };\n");
        g_print ("    property_changed (%s (prop_id));\n", ctPropertyID);
        g_print ("    update_modules();\n");
        /* reset triggers */
        g_print ("    switch (prop_id) {\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          {
            if (pi->pspec != "Trigger")
              continue;
            g_print ("    case PROP_%s:\n", pure_UPPER (pi->name));
            g_print ("      %s = FALSE;\n", pi->name.c_str());
            g_print ("    break;\n");
          }
        g_print ("    default: ;\n");
        g_print ("    };\n");
        g_print ("  }\n");

        /* editable_property() */
        g_print ("  virtual bool editable_property (%s prop_id, GParamSpec *pspec)\n", ctPropertyID);
        g_print ("  {\n");
        g_print ("    return true;\n");
        g_print ("  }\n");

        /* get_candidates() */
        g_print ("  virtual void get_candidates (%s prop_id, ::Bse::PropertyCandidatesHandle &pch, GParamSpec *pspec)\n", ctPropertyID);
        g_print ("  {\n");
        g_print ("  }\n");

        /* property_updated() */
        g_print ("  void property_updated (%s prop_id, guint64 tick_stamp, double prop_value, GParamSpec *pspec)\n", ctPropertyID);
        g_print ("  {\n");
        g_print ("    bool seen_change = false;\n");
        g_print ("    switch (prop_id) {\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          {
            if (!g_option_check (pi->literal_options.c_str(), "automate"))
              continue;
            g_print ("    case PROP_%s:\n", pure_UPPER (pi->name));
            g_print ("      if (tick_stamp >= ::std::max (last__%s, module_update_tick_stamp()))\n", pi->name.c_str());
            g_print ("        {\n");
            g_print ("          seen_change = true;\n");
            g_print ("          %s = prop_value;\n", pi->name.c_str());
            g_print ("        }\n");
            g_print ("      last__%s = tick_stamp;\n", pi->name.c_str());
            g_print ("    break;\n");
          }
        g_print ("    default: ;\n");
        g_print ("    };\n");
        g_print ("    if (seen_change &&\n");
        g_print ("        property_changed (%s (prop_id)))\n", ctPropertyID);
        g_print ("      update_modules();\n");
        g_print ("  }\n");

        /* static data */
        g_print ("private:\n");
        g_print ("  static struct StaticData {\n");
        g_print ("    int dummy;\n");
        for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
          {
            const gchar *sig_name = canonify_name (si->name, '_');
            g_print ("    guint signal_%s;\n", sig_name);
          }
        g_print ("  } static_data;\n");
        
        /* property-changed hooking */
        g_print ("protected:\n");
        g_print ("  virtual bool property_changed (%s) { return false; }\n", ctPropertyID);
        
        /* methods */
        for (vector<Method>::const_iterator mi = ci->methods.begin(); mi != ci->methods.end(); mi++)
          procs.push_back (&(*mi));
        
        /* destructor */
        g_print ("  virtual ~%s ()\n", ctNameBase);
        g_print ("  {\n");
        /* property deletion */
        for (vector<String>::const_iterator vi = destroy_jobs.begin(); vi != destroy_jobs.end(); vi++)
          g_print ("    %s;\n", vi->c_str());
        g_print ("  }\n");
        
        /* signal emission methods */
        g_print ("public:\n");
        for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
          {
            const gchar *sig_name = canonify_name (si->name, '_');
            g_print ("  void emit_%s (", sig_name);
            for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++)
              {
                if (ai != si->params.begin())
                  g_print (", ");
                g_print ("%s %s", TypeArg (ai->type), ai->name.c_str());
              }
            g_print (")\n");
            g_print ("  {\n");
            g_print ("    GValue args[1 + %zu];\n", si->params.size());
            g_print ("    args[0].g_type = 0, g_value_init (args + 0, %s);\n", make_TYPE_NAME (ci->name));
            g_print ("    g_value_set_object (args + 0, gobject());\n");
            guint i = 1;
            for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++, i++)
              {
                g_print ("    args[%u].g_type = 0, g_value_init (args + %u, %s);\n", i, i, make_TYPE_NAME (ai->type));
                g_print ("    %s (args + %u, %s);\n", func_value_set_param (ai->type), i, ai->name.c_str());
              }
            g_print ("    g_signal_emitv (args, static_data.signal_%s, 0, NULL);\n", sig_name);
            for (i = 0; i <= si->params.size(); i++)
              g_print ("    g_value_unset (args + %u);\n", i);
            g_print ("  }\n");
          }
        
        /* done */
        g_print ("};\n"); /* finish: class ... { }; */
      }
  }
  void
  generate_class_implementations (NamespaceHelper& nspace)
  {
    g_print ("\n\n/* class implementations */\n");
    for (vector<Class>::const_iterator ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
      {
        if (parser.fromInclude (ci->name))
          continue;
        nspace.setFromSymbol(ci->name);
        const char *ctName = pure_TypeName (ci->name);
        const char *ctNameBase = intern (ctName + String ("Base"));
        const char *ctPropertyID = intern (ctName + String ("PropertyID"));
        const char *nname = nspace.printable_form (ci->name);
        vector<String> destroy_jobs;
        
        /* class_init */
        g_print ("void\n");
        g_print ("%sBase::class_init (::Bse::CxxBaseClass *klass)\n", nname);
        g_print ("{\n");
        g_print ("  klass->set_accessors (::Bse::cxx_get_property_trampoline<%s, %s>,\n", ctNameBase, ctPropertyID);
        g_print ("                        ::Bse::cxx_set_property_trampoline<%s, %s>,\n", ctNameBase, ctPropertyID);
        g_print ("                        ::Bse::cxx_editable_property_trampoline<%s, %s>,\n", ctNameBase, ctPropertyID);
        g_print ("                        ::Bse::cxx_get_candidates_trampoline<%s, %s>,\n", ctNameBase, ctPropertyID);
        if (class_has_automation_properties (*ci))
          g_print ("                        ::Bse::cxx_property_updated_trampoline<%s, %s>);\n", ctNameBase, ctPropertyID);
        else
          g_print ("                        NULL);\n");
        for (vector<Param>::const_iterator pi = ci->properties.begin(); pi != ci->properties.end(); pi++)
          g_print ("  klass->add_param (PROP_%s, %s);\n", pure_UPPER (pi->name), typed_pspec_constructor (*pi).c_str());
        for (vector<Stream>::const_iterator si = ci->istreams.begin(); si != ci->istreams.end(); si++)
          g_print ("  klass->add_ichannel (%s, %s, %s, ICHANNEL_%s);\n",
                  intern_escape (si->ident), si->label.escaped().c_str(), si->blurb.escaped().c_str(), pure_UPPER (si->ident));
        for (vector<Stream>::const_iterator si = ci->jstreams.begin(); si != ci->jstreams.end(); si++)
          g_print ("  klass->add_jchannel (%s, %s, %s, JCHANNEL_%s);\n",
                  intern_escape (si->ident), si->label.escaped().c_str(), si->blurb.escaped().c_str(), pure_UPPER (si->ident));
        for (vector<Stream>::const_iterator si = ci->ostreams.begin(); si != ci->ostreams.end(); si++)
          g_print ("  klass->add_ochannel (%s, %s, %s, OCHANNEL_%s);\n",
                  intern_escape (si->ident), si->label.escaped().c_str(), si->blurb.escaped().c_str(), pure_UPPER (si->ident));
        for (vector<Method>::const_iterator si = ci->signals.begin(); si != ci->signals.end(); si++)
          {
            const gchar *sig_name = canonify_name (si->name, '_');
            const gchar *sig_string = canonify_name (si->name);
            g_print ("  static_data.signal_%s =\n      klass->add_signal (\"%s\", (GSignalFlags) 0, %zu",
                    sig_name, sig_string, si->params.size());
            for (vector<Param>::const_iterator ai = si->params.begin(); ai != si->params.end(); ai++)
              g_print (",\n                       %s", make_TYPE_NAME (ai->type));
            g_print (");\n");
          }
        g_print ("}\n");
      }
  }
  void
  generate_procedure_prototypes (NamespaceHelper&            nspace)
  {
    g_print ("\n\n/* procedure prototypes */\n");
    for (vector<const Method*>::const_iterator ppi = procs.begin(); ppi != procs.end(); ppi++)
      {
        const Method *mi = *ppi;
        if (parser.fromInclude (mi->name))
          continue;
        nspace.setFromSymbol(mi->name);

        g_print ("namespace Procedure {\n");
        g_print ("class %s;\n", pure_lower (mi->name));
        g_print ("#define %s\t\tBSE_CXX_DECLARED_PROC_TYPE (%s, %s)\n",
                make_TYPE_NAME (mi->name),
                nspace.namespaceOf (mi->name).c_str(),
                pure_lower (mi->name));
        g_print ("} // Procedure\n\n");
        push_type ("PROCEDURE", pure_lower (mi->name));
      }
  }
  void
  generate_procedure_implementations (NamespaceHelper &nspace)
  {
    g_print ("\n\n/* procedure implementations */\n");
    for (vector<const Method*>::const_iterator ppi = procs.begin(); ppi != procs.end(); ppi++)
      {
        const Method *mi = *ppi;
        if (parser.fromInclude (mi->name))
          continue;
        nspace.setFromSymbol(mi->name);
        const char *name = nspace.printable_form (mi->name);
        const Map<String, IString> &infos = mi->infos;
        bool is_void = mi->result.type == "void";
        g_print ("namespace Procedure {\n");
        g_print ("BSE_CXX_DECLARE_PROC (%s);\n", pure_lower (mi->name));
        g_print ("class %s {\n", pure_lower (mi->name));
        
        /* class Info strings */
        /* pixstream(), this is a bit of a hack, we make it a template rather than
         * a normal inline method to avoid huge images in debugging code
         */
        String icon = infos.get("icon");
        String pstream = "NULL";
        if (icon != "")
          {
            g_print ("  template<bool> static inline const unsigned char* inlined_pixstream();\n");
            images.push_back (Image (include_relative (icon, mi->file),
                                     String ("template<bool> const unsigned char*\n") +
                                     make_full_lower (mi->name) +
                                     "::inlined_pixstream()"));
            pstream = "inlined_pixstream<true>()";
          }
        g_print ("public:\n");
        g_print ("  static inline const unsigned char* pixstream () { return %s; }\n", pstream.c_str());
        g_print ("  static inline const char* options   () { return %s; }\n", infos.get("options").escaped().c_str());
        g_print ("  static inline const char* category  () { static const char *c = NULL;\n");
        g_print ("    return c ? c : c = sfi_category_concat (\"/Proc\", %s); }\n",
                infos.get("category").escaped(" ").c_str());    // untranslated
        g_print ("  static inline const char* i18n_category  () { static const char *c = NULL;\n");
        g_print ("    return c ? c : c = sfi_category_concat (\"/Proc\", %s); }\n",
                infos.get("category").escaped().c_str());       // translated
        g_print ("  static inline const char* blurb     () { return %s; }\n", infos.get("blurb").escaped().c_str());
        g_print ("  static inline const char* authors   () { return %s; }\n", infos.get("authors").escaped().c_str());
        g_print ("  static inline const char* license   () { return %s; }\n", infos.get("license").escaped().c_str());
        g_print ("  static inline const char* type_name () { return \"%s\"; }\n", make_scheme_name (mi->name));
        
        /* return type */
        g_print ("  static %s exec (", TypeRet (mi->result.type));
        /* args */
        for (vector<Param>::const_iterator ai = mi->params.begin(); ai != mi->params.end(); ai++)
          {
            if (ai != mi->params.begin())
              g_print (", ");
            g_print ("%s %s", TypeArg (ai->type), ai->name.c_str());
          }
        g_print (");\n");
        
        /* marshal */
        g_print ("  static BseErrorType marshal (BseProcedureClass *procedure,\n"
                "                               const GValue      *in_values,\n"
                "                               GValue            *out_values)\n");
        g_print ("  {\n");
        g_print ("    try {\n");
        if (!is_void)
          g_print ("      %s __return_value =\n", TypeRet (mi->result.type));
        g_print ("        exec (\n");
        int i = 0;
        for (vector<Param>::const_iterator pi = mi->params.begin(); pi != mi->params.end(); pi++)
          g_print ("              %s (in_values + %u)%c\n",
                  func_value_get_param (pi->type), i++,
                  &(*pi) == &(mi->params.back()) ? ' ' : ',');
        g_print ("             );\n");
        if (!is_void)
          g_print ("      %s (out_values, __return_value);\n", func_value_set_param (mi->result.type));
        g_print ("    } catch (std::exception &e) {\n");
        g_print ("      sfi_diag (\"%%s: %%s\", \"%s\", e.what());\n", name);
        g_print ("      return BSE_ERROR_PROC_EXECUTION;\n");
        g_print ("    } catch (...) {\n");
        g_print ("      sfi_diag (\"%%s: %%s\", \"%s\", \"uncaught exception\");\n", name);
        g_print ("      return BSE_ERROR_PROC_EXECUTION;\n");
        g_print ("    }\n");
        g_print ("    return BSE_ERROR_NONE;\n");
        g_print ("  }\n");
        
        /* init */
        g_print ("  static void init (BseProcedureClass *proc,\n"
                "                    GParamSpec       **in_pspecs,\n"
                "                    GParamSpec       **out_pspecs)\n");
        g_print ("  {\n");
        for (vector<Param>::const_iterator ai = mi->params.begin(); ai != mi->params.end(); ai++)
          g_print ("    *(in_pspecs++) = %s;\n", typed_pspec_constructor (*ai).c_str());
        if (!is_void)
          g_print ("    *(out_pspecs++) = %s;\n", typed_pspec_constructor (mi->result).c_str());
        g_print ("  }\n");
        
        /* done */
        g_print ("};\n"); /* finish: class ... { }; */
        g_print ("} // Procedure\n\n");
      }
  }
  bool
  run ()
  {
    g_print ("\n/*-------- begin %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());
    
    /* standard includes */
    g_print ("\n#include <bse/bsecxxplugin.hh>\n");
    
    /* reset auxillary structures */
    images.resize (0);
    procs.resize (0);

    /* setup namespace state */
    NamespaceHelper nsh(stdout);

    /* prototypes */
    generate_enum_prototypes (nsh);             /* adds to alltypes */
    generate_choice_prototypes (nsh);           
    generate_record_prototypes (nsh);           /* adds to alltypes */
    generate_sequence_prototypes (nsh);         /* adds to alltypes */
    generate_class_prototypes (nsh);            /* adds to alltypes */

    /* definitions */
    generate_enum_definitions (nsh);
    generate_sequence_definitions (nsh);
    generate_record_definitions (nsh);

    /* (type) declarations */
    generate_enum_declarations (nsh);
    generate_sequence_declarations (nsh);
    generate_record_declarations (nsh);

    /* procedure handling */
    for (vector<Method>::const_iterator mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
      procs.push_back (&(*mi));                 /* collect procedures */
    generate_procedure_prototypes (nsh);

    /* class bodies */
    generate_class_definitions (nsh);           /* adds to images, procs */

    /* implementations */
    generate_choice_implementations (nsh);
    generate_record_implementations (nsh);
    generate_sequence_implementations (nsh);
    generate_class_implementations (nsh);
    generate_procedure_implementations (nsh);   /* adds to images */

    /* alltypes macro */
    g_print ("\n\n/* %s type registrations */\n", alltypes_macro.c_str());
    if (alltypes_macro != "" && alltypes.size())
      {
        g_print ("#define BSE_CXX_REGISTER_ALL_TYPES_FROM_%s() \\\n", UPPER_CASE (alltypes_macro));
        for (vector<String>::const_iterator si = alltypes.begin(); si != alltypes.end(); si++)
          g_print ("  BSE_CXX_REGISTER_%s; \\\n", si->c_str());
        g_print ("  /* %s type registrations done */\n", alltypes_macro.c_str());
      }

    /* close namespace state */
    nsh.leaveAll();

    // g_print ("\nnamespace ... {\n"); // FIXME: do images need a namespace?

    /* image method implementations */
    for (vector<Image>::const_iterator ii = images.begin(); ii != images.end(); ii++)
      {
        g_print ("%s\n", ii->method.c_str());
        g_print ("{\n");
        gint estatus = 0;
        GError *error = NULL;
        gchar *out, *err = NULL;
        String cmd = String() + "gdk-pixbuf-csource " + "--name=local_pixstream " + ii->file;
        g_spawn_command_line_sync (cmd.c_str(), &out, &err, &estatus, &error);
        if (err && *err)
          g_printerr ("gdk-pixbuf-csource: %s", err);
        if (error || estatus)
          {
            if (error)
              g_printerr ("failed to convert image file \"%s\" with gdk-pixbuf-csource%c %s",
                          ii->file.c_str(), error ? ':' : ' ', error->message);
            exit (estatus & 255 ? estatus : 1);
          }
        g_clear_error (&error);
        g_free (err);
        g_print ("  %s\n", out);
        g_free (out);
        g_print ("  return local_pixstream;\n");
        g_print ("}\n");
      }

    // g_print ("\n}; // ...\n"); // FIXME: do images need a namespace?
    
    /* done */
    g_print ("\n/*-------- end %s generated code --------*/\n\n\n", Options::the()->sfidlName.c_str());
    return true;
  }
};

class LanguageBindingCoreCxxFactory : public Factory {
public:
  String option() const	      { return "--core-cxx"; }
  String description() const  { return "generate core C++ binding"; }

  CodeGenerator *create (const Parser& parser) const
  {
    return new LanguageBindingCoreCxx (parser);
  }
} static_factory;

class LanguageBindingPluginFactory : public Factory {
public:
  String option() const	      { return "--plugin"; }
  String description() const  { return "generate C++ plugin binding"; }

  CodeGenerator *create (const Parser& parser) const
  {
    return new LanguageBindingCoreCxx (parser);
  }
} static_factory2;

} // anon

/* vim:set ts=8 sts=2 sw=2: */
