/* SFI - Synthesis Fusion Kit Interface
 * Copyright (C) 2002-2007 Stefan Westerfeld
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
#include "sfidl-utils.hh"
#include "sfidl-factory.hh"
#include "sfidl-generator.hh"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

namespace {

using namespace Sfidl;
using std::make_pair;

class CodeGeneratorCoreC : public CodeGenerator {
  const char*
  intern (const String &str)
  {
    return g_intern_string (str.c_str());
  }
  const char*
  make_type_id_symbol (const String &type_name,
                       const String &append = "")
  {
    vector<String> insertions;
    insertions.push_back ("_type_id_");
    String s = rename (ABSOLUTE, type_name, lower, "_", insertions, lower, "_");
    s += append;
    return intern (s);
  }
  const char*
  make_TYPE_MACRO (const String &type_name,
                   const String &append = "")
  {
    vector<String> insertions;
    insertions.push_back ("TYPE");
    String s = rename (ABSOLUTE, type_name, UPPER, "_", insertions, UPPER, "_");
    s += append;
    return intern (s);
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
  String generateInitFunction;
  void
  printInfoStrings (const String&              name,
                    const Map<String,IString> &infos)
  {
    g_print ("static const gchar *%s[] = {\n", name.c_str());
    
    Map<String,IString>::const_iterator ii;
    for (ii = infos.begin(); ii != infos.end(); ii++)
      g_print ("  \"%s=%s\",\n", ii->first.c_str(), ii->second.c_str());
    
    g_print ("  NULL,\n");
    g_print ("};\n");
  }
  
  void
  help()
  {
    CodeGenerator::help();
    g_printerr (" --init <name>               set the name of the init function\n");
  }
  OptionVector
  getOptions()
  {
    OptionVector opts = CodeGenerator::getOptions();
    
    opts.push_back (make_pair ("--init", true));
    
    return opts;
  }
  void
  setOption (const String &option,
             const String &value)
  {
    if (option == "--init")
      {
        generateInitFunction = value;
      }
    else
      {
        CodeGenerator::setOption (option, value);
      }
  }

  const char*
  TypeName (const String &type_name,
            const String &append = "")
  {
    vector<String> empty;
    String s = rename (ABSOLUTE, type_name, Capitalized, "", empty, Capitalized, "");
    s += append;
    return intern (s);
  }
  const char*
  TypeField (const String &type)
  {
    switch (parser.typeOf (type))
      {
      case VOID:        return "void";
      case BOOL:        return "SfiBool";
      case INT:         return "SfiInt";
      case NUM:         return "SfiNum";
      case REAL:        return "SfiReal";
      case CHOICE:      return TypeName (type);
      case STRING:      return "gchar*";
      case BBLOCK:      return "SfiBBlock*";
      case FBLOCK:      return "SfiFBlock*";
      case SFIREC:      return "SfiRec*";
      case RECORD:      return TypeName (type, "*");
      case SEQUENCE:    return TypeName (type, "*");
      case OBJECT:      return TypeName (type, "*");
      default:          g_assert_not_reached(); return NULL;
      }
  }
  const char*
  TypeArg (const String &type)
  {
    switch (parser.typeOf (type))
      {
      case STRING:      return "const char*";
      default:          return TypeField (type);
      }
  }
  const char*
  TypeRet (const String& type)
  {
    switch (parser.typeOf (type))
      {
      default:          return TypeArg (type);
      }
  }
  const char*
  cxx_handle (const String &type,
              const String &var)
  {
    switch (parser.typeOf (type))
      {
      case RECORD:
        {
          String s, cxxtype = String() + make_fqtn (type) + "Handle";
          /* generate: (var ? FooHandle(*var) : FooHandle(INIT_NULL)) */
          s = "(" +
              var + " ? " +
              cxxtype + " (*hack_cast (" + var + ")) : " +
              cxxtype + " (Sfi::INIT_NULL)" +
              ")";
          return intern (s.c_str());
        }
        // FIXME: this lacks variants for sequences, *blocks, etc.
      default:          return intern (var);
      }
  }
  String
  construct_pspec (const Param &pdef)
  {
    String pspec;
    const String group = (pdef.group != "") ? pdef.group.escaped() : "NULL";
    String pname, parg;
    switch (parser.typeOf (pdef.type))
      {
      case CHOICE:
        pname = "Choice";
        parg = makeLowerName (pdef.type) + "_get_values()";
        break;
      case RECORD:
        pname = "Record";
        parg = makeLowerName (pdef.type) + "_fields";
        break;
      case SEQUENCE:
        pname = "Sequence";
        parg = makeLowerName (pdef.type) + "_content";
        break;
      case OBJECT:
        pname = "Object";
        break;
      default:
        pname = pdef.pspec;
        break;
      }
    pspec = "sfidl_pspec_" + pname;
    if (parg != "")
      parg = String (", ") + parg;
    if (pdef.args == "")
      pspec += "_default (" + group + ",\"" + pdef.name + parg + "\")";
    else
      pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + parg + ")";
    return pspec;
  }

  void
  generate_enum_type_id_prototypes ()
  {
    g_print ("\n\n/* enum type ids */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        g_print ("extern GType %s;\n", make_type_id_symbol (ei->name));
      }
  }
  void
  generate_enum_type_id_declarations ()
  {
    g_print ("\n\n/* enum type ids */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        g_print ("GType %s = 0;\n", make_type_id_symbol (ei->name));
      }
  }
  void
  generate_enum_type_id_initializations ()
  {
    g_print ("\n\n  /* enum type ids */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        g_print ("  %s = %s;\n", make_type_id_symbol (ei->name), make_TYPE_MACRO (ei->name));
      }
  }
  void
  generate_enum_type_macros ()
  {
    g_print ("\n\n/* enum type macros */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        g_print ("#define %s\t\t(%s)\n", make_TYPE_MACRO (ei->name), make_type_id_symbol (ei->name));
      }
  }
  void
  generate_enum_definitions ()
  {
    g_print ("\n\n/* enums */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        String mname = makeMixedName (ei->name);
        String lname = makeLowerName (ei->name);
        g_print ("\ntypedef enum {\n");
        for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
          {
            /* don't export server side assigned choice values to the client */
            gint value = ci->value;
            String ename = makeUpperName (ci->name);
            g_print ("  %s = %d,\n", ename.c_str(), value);
          }
        g_print ("} %s;\n", mname.c_str());
      }
  }
  void
  generate_enum_value_array ()
  {
    g_print ("\n\n/* enum values */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        String name = makeLowerName (ei->name);
        g_print ("static const GEnumValue %s_value[%zd] = {\n", name.c_str(), ei->contents.size() + 1); // FIXME: i18n
        for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
          {
            String ename = makeUpperName (ci->name);
            g_print ("  { %d, \"%s\", \"%s\" },\n", ci->value, ename.c_str(), ci->label.c_str());
          }
        g_print ("  { 0, NULL, NULL }\n");
        g_print ("};\n");
      }
  }
  void
  generate_enum_method_prototypes ()
  {
    g_print ("\n\n/* enum functions */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        g_print ("SfiChoiceValues %s_get_values (void);\n", makeLowerName (ei->name).c_str());
      }
  }
  void
  generate_enum_method_implementations ()
  {
    int enumCount = 0;
    g_print ("\n\n/* enum functions */\n");
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name))
          continue;
        String name = makeLowerName (ei->name);
        g_print ("SfiChoiceValues\n");
        g_print ("%s_get_values (void)\n", makeLowerName (ei->name).c_str());
        g_print ("{\n");
        g_print ("  static SfiChoiceValue values[%zu];\n", ei->contents.size());
        g_print ("  static const SfiChoiceValues choice_values = {\n");
        g_print ("    G_N_ELEMENTS (values), values,\n");
        g_print ("  };\n");
        g_print ("  if (!values[0].choice_ident)\n    {\n");
        int i = 0;
        for (vector<ChoiceValue>::const_iterator vi = ei->contents.begin(); vi != ei->contents.end(); i++, vi++)
          {
            g_print ("      values[%u].choice_ident = \"%s\";\n", i, makeUpperName (vi->name).c_str());
            g_print ("      values[%u].choice_label = %s;\n", i, vi->label.escaped().c_str());
            g_print ("      values[%u].choice_blurb = %s;\n", i, vi->blurb.escaped().c_str());
          }
        g_print ("  }\n");
        g_print ("  return choice_values;\n");
        g_print ("}\n");
        
        g_print ("GType %s = 0;\n", make_TYPE_MACRO (ei->name));
        g_print ("\n");
        
        enumCount++;
      }
    if (enumCount)
      {
        g_print ("static void\n");
        g_print ("choice2enum (const GValue *src_value,\n");
        g_print ("             GValue       *dest_value)\n");
        g_print ("{\n");
        g_print ("  sfi_value_choice2enum (src_value, dest_value, NULL);\n");
        g_print ("}\n");
      }
  }
  void
  generate_record_prototypes ()
  {
    g_print ("\n\n/* record typedefs */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        String mname = makeMixedName (ri->name);
        g_print ("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
      }
  }
  void
  generate_record_definitions ()
  {
    g_print ("\n\n/* records */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name)) continue;
        
        String mname = makeMixedName (ri->name.c_str());
        
        g_print ("struct _%s {\n", mname.c_str());
        for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
          {
            g_print ("  %s %s;\n", TypeField (pi->type), pi->name.c_str());
          }
        g_print ("};\n");
      }
  }
  void
  generate_record_type_id_prototypes ()
  {
    g_print ("\n\n/* record type ids */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        g_print ("extern GType %s;\n", make_type_id_symbol (ri->name));
      }
  }
  void
  generate_record_type_id_declarations ()
  {
    g_print ("\n\n/* record type ids */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        g_print ("GType %s = 0;\n", make_type_id_symbol (ri->name));
      }
  }
  void
  generate_record_type_id_initializations ()
  {
    g_print ("\n\n  /* record type ids */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        g_print ("  %s = %s;\n", make_type_id_symbol (ri->name), make_TYPE_MACRO (ri->name));
      }
  }
  void
  generate_record_type_macros ()
  {
    g_print ("\n\n/* record type macros */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        g_print ("#define %s\t\t(%s)\n", make_TYPE_MACRO (ri->name), make_type_id_symbol (ri->name));
      }
  }
  void
  generate_record_method_prototypes ()
  {
    g_print ("\n\n/* record functions */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name)) continue;
        
        String ret = TypeRet (ri->name);
        String arg = TypeArg (ri->name);
        String lname = makeLowerName (ri->name.c_str());
        
        g_print ("SfiRecFields %s_get_fields (void);\n", lname.c_str());
        g_print ("%s %s_new (void);\n", ret.c_str(), lname.c_str());
        g_print ("%s %s_copy_shallow (%s rec);\n", ret.c_str(), lname.c_str(), arg.c_str());
        g_print ("%s %s_from_rec (SfiRec *sfi_rec);\n", ret.c_str(), lname.c_str());
        g_print ("SfiRec *%s_to_rec (%s rec);\n", lname.c_str(), arg.c_str());
        g_print ("void %s_free (%s rec);\n", lname.c_str(), arg.c_str());
        g_print ("\n");
      }
  }
  void
  generate_record_hack_cast_implementations ()
  {
    g_print ("\n\n/* record functions */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        String ret = TypeRet (ri->name);
        const gchar *type = make_fqtn (ri->name);

        g_print ("static inline %s\n", ret.c_str());
        g_print ("hack_cast (%s *cxxstruct)\n", type);
        g_print ("{\n");
        g_print ("  return reinterpret_cast<%s> (cxxstruct);\n", ret.c_str());
        g_print ("}\n");
        g_print ("static inline %s*\n", type);
        g_print ("hack_cast (%s cstruct)\n", ret.c_str());
        g_print ("{\n");
        g_print ("  return reinterpret_cast< %s*> (cstruct);\n", type);
        g_print ("}\n");
      }
  }
  void
  generate_record_method_implementations ()
  {
    g_print ("\n\n/* record functions */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        String ret = TypeRet (ri->name);
        String arg = TypeArg (ri->name);
        String lname = makeLowerName (ri->name.c_str());
        String mname = makeMixedName (ri->name.c_str());

        const gchar *type = make_fqtn (ri->name);

        g_print ("SfiRecFields\n");
        g_print ("%s_get_fields (void)\n", lname.c_str());
        g_print ("{\n");
        g_print ("  return %s::get_fields ();\n", type);
        g_print ("}\n");

        g_print ("%s\n", ret.c_str());
        g_print ("%s_new (void)\n", lname.c_str());
        g_print ("{\n");
        g_print ("  %sHandle rh (Sfi::INIT_DEFAULT); \n", type);
        g_print ("  return hack_cast (rh.steal());\n");
        g_print ("}\n");
        
        g_print ("%s\n", ret.c_str());
        g_print ("%s_copy_shallow (%s cstruct)\n", lname.c_str(), arg.c_str());
        g_print ("{\n");
        g_print ("  %sHandle rh;\n", type);
        g_print ("  rh.set_boxed (hack_cast (cstruct));\n");
        g_print ("  return hack_cast (rh.steal());\n");
        g_print ("}\n");
        
        g_print ("%s\n", ret.c_str());
        g_print ("%s_from_rec (SfiRec *rec)\n", lname.c_str());
        g_print ("{\n");
        g_print ("  %sHandle rh = %s::from_rec (rec);\n", type, type);
        g_print ("  return hack_cast (rh.steal());\n");
        g_print ("}\n");
        
        g_print ("SfiRec*\n");
        g_print ("%s_to_rec (%s cstruct)\n", lname.c_str(), arg.c_str());
        g_print ("{\n");
        g_print ("  %sHandle rh;\n", type);
        g_print ("  rh.set_boxed (hack_cast (cstruct));\n");
        g_print ("  return %s::to_rec (rh);\n", type);
        g_print ("}\n");
        
        g_print ("void\n");
        g_print ("%s_free (%s cstruct)\n", lname.c_str(), arg.c_str());
        g_print ("{\n");
        g_print ("  %sHandle rh;\n", type);
        g_print ("  rh.take (hack_cast (cstruct));\n");
        g_print ("}\n");
        g_print ("\n");
      }
  }
  void
  generate_record_converter_implementations ()
  {
    g_print ("\n\n/* record converters */\n");
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name))
          continue;
        
        String name = makeLowerName (ri->name);
        
        g_print ("static GParamSpec *%s_field[%zd];\n", name.c_str(), ri->contents.size());
        g_print ("SfiRecFields %s_fields = { %zd, %s_field };\n", name.c_str(), ri->contents.size(), name.c_str());
        
        String mname = makeMixedName (ri->name);
        
        g_print ("static void\n");
        g_print ("%s_boxed2rec (const GValue *src_value, GValue *dest_value)\n", name.c_str());
        g_print ("{\n");
        g_print ("  gpointer boxed = g_value_get_boxed (src_value);\n");
        g_print ("  sfi_value_take_rec (dest_value, boxed ? %s_to_rec (boxed) : NULL);\n", name.c_str());
        g_print ("}\n");
        
        g_print ("static void\n");
        g_print ("%s_rec2boxed (const GValue *src_value, GValue *dest_value)\n", name.c_str());
        g_print ("{\n");
        g_print ("  SfiRec *rec = sfi_value_get_rec (src_value);\n");
        g_print ("  g_value_take_boxed (dest_value,\n");
        g_print ("    rec ? %s_from_rec (rec) : NULL);\n", name.c_str());
        g_print ("}\n");
        
        printInfoStrings (name + "_info_strings", ri->infos);
        g_print ("static SfiBoxedRecordInfo %s_boxed_info = {\n", name.c_str());
        g_print ("  \"%s\",\n", mname.c_str());
        g_print ("  { %zd, %s_field },\n", ri->contents.size(), name.c_str());
        g_print ("  %s_boxed2rec,\n", name.c_str());
        g_print ("  %s_rec2boxed,\n", name.c_str());
        g_print ("  %s_info_strings\n", name.c_str());
        g_print ("};\n");
        g_print ("GType %s = 0;\n", make_TYPE_MACRO (ri->name));
      }
  }
  void
  generate_sequence_prototypes ()
  {
    g_print ("\n\n/* sequence typedefs */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        String mname = makeMixedName (si->name);
        g_print ("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
      }
  }
  void
  generate_sequence_definitions ()
  {
    g_print ("\n\n/* sequences */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name)) continue;
        
        String mname = makeMixedName (si->name.c_str());
        String array = String (TypeField (si->content.type)) + "*";
        String elements = si->content.name;
        
        g_print ("struct _%s {\n", mname.c_str());
        g_print ("  guint n_%s;\n", elements.c_str ());
        g_print ("  %s %s;\n", array.c_str(), elements.c_str());
        g_print ("};\n");
      }
  }
  void
  generate_sequence_type_id_prototypes ()
  {
    g_print ("\n\n/* sequence type ids */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        g_print ("extern GType %s;\n", make_type_id_symbol (si->name));
      }
  }
  void
  generate_sequence_type_id_declarations ()
  {
    g_print ("\n\n/* sequence type ids */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        g_print ("GType %s = 0;\n", make_type_id_symbol (si->name));
      }
  }
  void
  generate_sequence_type_id_initializations ()
  {
    g_print ("\n\n  /* sequence type ids */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        g_print ("  %s = %s;\n", make_type_id_symbol (si->name), make_TYPE_MACRO (si->name));
      }
  }
  void
  generate_sequence_type_macros ()
  {
    g_print ("\n\n/* sequence type macros */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        g_print ("#define %s\t\t(%s)\n", make_TYPE_MACRO (si->name), make_type_id_symbol (si->name));
      }
  }
  void
  generate_sequence_method_prototypes ()
  {
    g_print ("\n\n/* sequence functions */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name)) continue;

        String ret = TypeRet (si->name);
        String arg = TypeArg (si->name);
        String element = TypeArg (si->content.type);
        String lname = makeLowerName (si->name.c_str());

        g_print ("GParamSpec* %s_get_element (void);\n", lname.c_str());
        g_print ("%s %s_new (void);\n", ret.c_str(), lname.c_str());
        g_print ("void %s_append (%s seq, %s element);\n", lname.c_str(), arg.c_str(), element.c_str());
        g_print ("%s %s_copy_shallow (%s seq);\n", ret.c_str(), lname.c_str(), arg.c_str());
        g_print ("%s %s_from_seq (SfiSeq *sfi_seq);\n", ret.c_str(), lname.c_str());
        g_print ("SfiSeq *%s_to_seq (%s seq);\n", lname.c_str(), arg.c_str());
        g_print ("void %s_resize (%s seq, guint new_size);\n", lname.c_str(), arg.c_str());
        g_print ("void %s_free (%s seq);\n", lname.c_str(), arg.c_str());
        g_print ("\n");
      }
  }
  void
  generate_sequence_hack_cast_implementations ()
  {
    g_print ("\n\n/* sequence C <-> C++ casts */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        String ret = TypeRet (si->name);
        const gchar *type = make_fqtn (si->name);

        /* the cast functions take an extra unused sequence argument, to distinguish
         * two sequences A and B which both have the same CSeq type (e.g. SfiInt and SfiNote).
         */
        g_print ("static inline %s\n", ret.c_str());
        g_print ("hack_cast (const %s &unused, %s::CSeq *cxxseq)\n", type, type);
        g_print ("{\n");
        g_print ("  return reinterpret_cast<%s> (cxxseq);\n", ret.c_str());
        g_print ("}\n");
        g_print ("static inline %s::CSeq*\n", type);
        g_print ("hack_cast (%s cseq)\n", ret.c_str());
        g_print ("{\n");
        g_print ("  return reinterpret_cast< %s::CSeq*> (cseq);\n", type);
        g_print ("}\n");
      }
  }
  void
  generate_sequence_method_implementations ()
  {
    g_print ("\n\n/* sequence functions */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name))
          continue;
        String ret = TypeRet (si->name);
        String arg = TypeArg (si->name);
        String element = TypeArg (si->content.type);
        String elements = si->content.name;
        String lname = makeLowerName (si->name.c_str());
        
        const gchar *type = make_fqtn (si->name);

        g_print ("GParamSpec*\n");
        g_print ("%s_get_element (void)\n", lname.c_str());
        g_print ("{\n");
        g_print ("  return %s::get_element ();\n", type);
        g_print ("}\n");

        g_print ("%s\n", ret.c_str());
        g_print ("%s_new (void)\n", lname.c_str());
        g_print ("{\n");
        g_print ("  %s sh (0);\n", type);
        g_print ("  return hack_cast (sh, sh.steal());\n");
        g_print ("}\n");
        
        g_print ("void\n");
        g_print ("%s_append (%s cseq, %s element)\n", lname.c_str(), arg.c_str(), element.c_str());
        g_print ("{\n");
        g_print ("  g_return_if_fail (cseq != NULL);\n");
        g_print ("  %s sh (0);\n", type);
        g_print ("  sh.take (hack_cast (cseq));\n");
        g_print ("  sh += %s;\n", cxx_handle (si->content.type, "element"));
        g_print ("  sh.steal(); /* prevent cseq deletion */\n");
        g_print ("}\n");
        
        g_print ("%s\n", ret.c_str());
        g_print ("%s_copy_shallow (%s cseq)\n", lname.c_str(), arg.c_str());
        g_print ("{\n");
        g_print ("  %s sh (0);\n", type);
        g_print ("  sh.set_boxed (hack_cast (cseq));\n");
        g_print ("  return hack_cast (sh, sh.steal());\n");
        g_print ("}\n");
        
        g_print ("%s\n", ret.c_str());
        g_print ("%s_from_seq (SfiSeq *seq)\n", lname.c_str());
        g_print ("{\n");
        g_print ("  %s sh = %s::from_seq (seq);\n", type, type);
        g_print ("  return hack_cast (sh, sh.steal());\n");
        g_print ("}\n");
        
        g_print ("SfiSeq*\n");
        g_print ("%s_to_seq (%s cseq)\n", lname.c_str(), arg.c_str());
        g_print ("{\n");
        g_print ("  %s sh (0);\n", type);
        g_print ("  sh.take (hack_cast (cseq));\n");
        g_print ("  SfiSeq *seq = %s::to_seq (sh);\n", type);
        g_print ("  sh.steal(); /* prevent cseq deletion */\n");
        g_print ("  return seq;\n");
        g_print ("}\n");
        
        g_print ("void\n");
        g_print ("%s_resize (%s cseq, guint n)\n", lname.c_str(), arg.c_str());
        g_print ("{\n");
        g_print ("  g_return_if_fail (cseq != NULL);\n");
        g_print ("  %s sh (0);\n", type);
        g_print ("  sh.take (hack_cast (cseq));\n");
        g_print ("  sh.resize (n);\n");
        g_print ("  sh.steal(); /* prevent cseq deletion */\n");
        g_print ("}\n");

        g_print ("void\n");
        g_print ("%s_free (%s cseq)\n", lname.c_str(), arg.c_str());
        g_print ("{\n");
        g_print ("  %s sh (0);\n", type);
        g_print ("  sh.take (hack_cast (cseq));\n");
        g_print ("}\n");
        g_print ("\n");
      }
  }
  void
  generate_sequence_converter_implementations ()
  {
    g_print ("\n\n/* sequence converters */\n");
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name)) continue;
        
        String name = makeLowerName (si->name);
        
        g_print ("static GParamSpec *%s_content;\n", name.c_str());
        
        String mname = makeMixedName (si->name);
        
        g_print ("static void\n");
        g_print ("%s_boxed2seq (const GValue *src_value, GValue *dest_value)\n", name.c_str());
        g_print ("{\n");
        g_print ("  gpointer boxed = g_value_get_boxed (src_value);\n");
        g_print ("  sfi_value_take_seq (dest_value, boxed ? %s_to_seq (boxed) : NULL);\n", name.c_str());
        g_print ("}\n");
        
        g_print ("static void\n");
        g_print ("%s_seq2boxed (const GValue *src_value, GValue *dest_value)\n", name.c_str());
        g_print ("{\n");
        g_print ("  SfiSeq *seq = sfi_value_get_seq (src_value);\n");
        g_print ("  g_value_take_boxed (dest_value,\n");
        g_print ("    seq ? %s_from_seq (seq) : NULL);\n", name.c_str());
        g_print ("}\n");
        
        printInfoStrings (name + "_info_strings", si->infos);
        g_print ("static SfiBoxedSequenceInfo %s_boxed_info = {\n", name.c_str());
        g_print ("  \"%s\",\n", mname.c_str());
        g_print ("  NULL, /* %s_content */\n", name.c_str());
        g_print ("  %s_boxed2seq,\n", name.c_str());
        g_print ("  %s_seq2boxed,\n", name.c_str());
        g_print ("  %s_info_strings\n", name.c_str());
        g_print ("};\n");
        g_print ("GType %s = 0;\n", make_TYPE_MACRO (si->name));
      }
  }
  void
  generate_init_function ()
  {
    bool first = true;
    g_print ("\n\n/* type initialization function */\n");
    g_print ("static void\n%s (void)\n", generateInitFunction.c_str());
    g_print ("{\n");
    
    /*
     * It is important to follow the declaration order of the idl file here, as for
     * instance a Param inside a record might come from a sequence, and a Param
     * inside a Sequence might come from a record - to avoid using yet-unitialized
     * Params, we follow the getTypes() 
     */
    vector<String>::const_iterator ti;
    
    for(ti = parser.getTypes().begin(); ti != parser.getTypes().end(); ti++)
      {
        if (parser.fromInclude (*ti)) continue;
        
        if (parser.isRecord (*ti) || parser.isSequence (*ti))
          {
            if (!first)
              g_print ("\n");
            first = false;
          }
        if (parser.isRecord (*ti))
          {
            const Record& rdef = parser.findRecord (*ti);
            
            String name = makeLowerName (rdef.name);
            int f = 0;
            
            for (vector<Param>::const_iterator pi = rdef.contents.begin(); pi != rdef.contents.end(); pi++, f++)
              {
                if (generateIdlLineNumbers)
                  g_print ("#line %u \"%s\"\n", pi->line, parser.fileName().c_str());
                g_print ("  %s_field[%d] = %s;\n", name.c_str(), f, construct_pspec (*pi).c_str());
              }
          }
        if (parser.isSequence (*ti))
          {
            const Sequence& sdef = parser.findSequence (*ti);
            
            String name = makeLowerName (sdef.name);
            
            if (generateIdlLineNumbers)
              g_print ("#line %u \"%s\"\n", sdef.content.line, parser.fileName().c_str());
            g_print ("  %s_content = %s;\n", name.c_str(), construct_pspec (sdef.content).c_str());
          }
      }
    for (vector<Choice>::const_iterator ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
      {
        if (parser.fromInclude (ei->name)) continue;
        
        String gname = make_TYPE_MACRO (ei->name);
        String name = makeLowerName(ei->name);
        String mname = makeMixedName(ei->name);
        
        g_print ("  %s = g_enum_register_static (\"%s\", %s_value);\n", gname.c_str(),
                mname.c_str(), name.c_str());
        g_print ("  g_value_register_transform_func (SFI_TYPE_CHOICE, %s, choice2enum);\n",
                gname.c_str());
        g_print ("  g_value_register_transform_func (%s, SFI_TYPE_CHOICE,"
                " sfi_value_enum2choice);\n", gname.c_str());
      }
    for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
      {
        if (parser.fromInclude (ri->name)) continue;
        
        String gname = make_TYPE_MACRO (ri->name);
        String name = makeLowerName(ri->name);
        
        g_print ("  %s = sfi_boxed_make_record (&%s_boxed_info,\n", gname.c_str(), name.c_str());
        g_print ("    (GBoxedCopyFunc) %s_copy_shallow,\n", name.c_str());
        g_print ("    (GBoxedFreeFunc) %s_free);\n", name.c_str());
      }
    for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
      {
        if (parser.fromInclude (si->name)) continue;
        
        String gname = make_TYPE_MACRO (si->name);
        String name = makeLowerName(si->name);
        
        g_print ("  %s_boxed_info.element = %s_content;\n", name.c_str(), name.c_str());
        g_print ("  %s = sfi_boxed_make_sequence (&%s_boxed_info,\n", gname.c_str(), name.c_str());
        g_print ("    (GBoxedCopyFunc) %s_copy_shallow,\n", name.c_str());
        g_print ("    (GBoxedFreeFunc) %s_free);\n", name.c_str());
      }
    g_print ("}\n");
  }
  
public:
  CodeGeneratorCoreC (const Parser& parser) :
    CodeGenerator (parser)
  {
  }

  bool
  run ()
  {
    g_print ("\n/*-------- begin %s generated code --------*/\n\n\n", options.sfidlName.c_str());
    if (generateSource)
      g_print ("#include <string.h>\n");
    
    if (generateHeader)
      {
        generate_enum_definitions ();
        generate_record_prototypes ();
        generate_sequence_prototypes ();
        generate_sequence_definitions ();
        generate_record_definitions ();
        generate_record_method_prototypes ();
        generate_sequence_method_prototypes ();
        generate_enum_method_prototypes ();
        generate_enum_type_id_prototypes ();
        generate_record_type_id_prototypes ();
        generate_sequence_type_id_prototypes ();
        g_print ("\n#ifndef __cplusplus\n");
        generate_enum_type_macros ();
        generate_record_type_macros ();
        generate_sequence_type_macros ();
        g_print ("\n#endif\n");
      }

    if (generateSource)
      {
        // generate_enum_value_array ();
        // generate_enum_method_implementations ();
        generate_record_hack_cast_implementations ();
        generate_sequence_hack_cast_implementations ();
        generate_record_method_implementations ();
        generate_sequence_method_implementations ();
        generate_enum_type_id_declarations ();
        generate_record_type_id_declarations ();
        generate_sequence_type_id_declarations ();
        // generate_record_converter_implementations ();
        // generate_sequence_converter_implementations ();
        // printChoiceConverters ();
        if (generateInitFunction != "")
          {     // generate_init_function();
            g_print ("\n\n/* type initialization function */\n");
            g_print ("static void\n%s (void)\n{\n", generateInitFunction.c_str());
            generate_enum_type_id_initializations ();
            generate_record_type_id_initializations ();
            generate_sequence_type_id_initializations ();
            g_print ("}\n");
          }
      }

    g_print ("\n/*-------- end %s generated code --------*/\n\n\n", options.sfidlName.c_str());
    return true;
  }
};

class CoreCFactory : public Factory {
public:
  String option() const	      { return "--core-c"; }
  String description() const  { return "generate core C language binding"; }
  
  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorCoreC (parser);
  }
} core_c_factory;

} // anon

/* vim:set ts=8 sts=2 sw=2: */
