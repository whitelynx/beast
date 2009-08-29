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
#include "sfidl-clientcxx.hh"
#include "sfidl-factory.hh"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "sfidl-namespace.hh"
#include "sfidl-options.hh"
#include "sfidl-parser.hh"
#include "sfiparams.h" /* scatId (SFI_SCAT_*) */

using namespace Sfidl;
using std::make_pair;

String
CodeGeneratorClientCxx::typeArg (const String& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "const Sfi::String&";
      case RECORD:    return "const " + type + "Handle&";
      case SEQUENCE:  return "const " + type + "&";
      case CHOICE:    return type;
      case OBJECT:    return type;
      default:	      return CodeGeneratorCBase::typeArg (type);
    }
}

String
CodeGeneratorClientCxx::typeField (const String& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "Sfi::String";
      case RECORD:    return type + "Handle";
      case CHOICE:
      case OBJECT:
      case SEQUENCE:  return type;
      default:	      return CodeGeneratorCBase::typeArg (type);
    }
}

String
CodeGeneratorClientCxx::typeRet (const String& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "Sfi::String";
      case RECORD:    return type + "Handle";
      case CHOICE:
      case OBJECT:
      case SEQUENCE:  return type;
      default:	      return CodeGeneratorCBase::typeArg (type);
    }
}

String
CodeGeneratorClientCxx::funcNew (const String& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "";
      default:	      return CodeGeneratorCBase::funcNew (type);
    }
}

String
CodeGeneratorClientCxx::funcCopy (const String& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "";
      default:	      return CodeGeneratorCBase::funcCopy (type);
    }
}

String
CodeGeneratorClientCxx::funcFree (const String& type)
{
  switch (parser.typeOf (type))
    {
      case STRING:    return "";
      default:	      return CodeGeneratorCBase::funcFree (type);
    }
}

String CodeGeneratorClientCxx::createTypeCode (const String& type, const String& name, 
				               TypeCodeModel model)
{
  /* FIXME: parameter validation */
  switch (parser.typeOf (type))
    {
      case STRING:
	switch (model)
	  {
	    case MODEL_TO_VALUE:    return "sfi_value_string ("+name+".c_str())";
	    case MODEL_FROM_VALUE:  return "::Sfi::String::value_get_string ("+name+")";
	    case MODEL_VCALL:       return "sfi_glue_vcall_string";
	    case MODEL_VCALL_ARG:   return "'" + scatId (SFI_SCAT_STRING) + "', "+name+".c_str(),";
	    case MODEL_VCALL_CARG:  return "";
	    case MODEL_VCALL_CONV:  return "";
	    case MODEL_VCALL_CFREE: return "";
	    case MODEL_VCALL_RET:   return "std::string";
	    case MODEL_VCALL_RCONV: return name;
	    case MODEL_VCALL_RFREE: return "";
	  }
	  break;
      case RECORD:
	switch (model)
	  {
	    case MODEL_TO_VALUE:    return "sfi_value_new_take_rec ("+type+"::to_rec ("+name+"))";
	    case MODEL_FROM_VALUE:  return type + "::from_rec (sfi_value_get_rec ("+name+"))";
	    case MODEL_VCALL_CONV:  return type + "::to_rec ("+name+")";
	    case MODEL_VCALL_RCONV: return type + "::from_rec ("+name+")";
	    case MODEL_VCALL_RFREE: return "";
	    default: ;
	  }
	  break;
      case SEQUENCE:
	switch (model)
	  {
	    case MODEL_TO_VALUE:    return "sfi_value_new_take_seq ("+type+"::to_seq ("+name+"))";
	    case MODEL_FROM_VALUE:  return type + "::from_seq (sfi_value_get_seq ("+name+"))";
	    case MODEL_VCALL_CONV:  return type + "::to_seq ("+name+")";
	    case MODEL_VCALL_RCONV: return type + "::from_seq ("+name+")";
	    case MODEL_VCALL_RFREE: return "";
	    default: ;
	  }
	break;
      case OBJECT:
	switch (model)
	  {
	    case MODEL_VCALL_ARG:
	      {
		/* this is a bit hacky */
		if (name == "_object_id")
		  return "'" + scatId (SFI_SCAT_PROXY) + "', "+name+",";
		else
		  return "'" + scatId (SFI_SCAT_PROXY) + "', "+name+"._proxy(),";
	      }
	    default: ;
	  }
      default: ;
    }
  return CodeGeneratorCBase::createTypeCode (type, name, model);
}

static const char*
cUC_NAME (const String &cstr) // FIXME: need mammut renaming function
{
  return g_intern_string (cstr.c_str());
}

void
CodeGeneratorClientCxx::printChoicePrototype (NamespaceHelper& nspace)
{
  g_print  ("\n/* choice prototypes */\n");
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;
      nspace.setFromSymbol(ci->name);
      String name = nspace.printableForm (ci->name);
      g_print  ("\n");
      g_print  ("static inline SfiChoiceValues %s_choice_values();\n", name.c_str());
    }
}

void
CodeGeneratorClientCxx::printChoiceImpl (NamespaceHelper& nspace)
{
  g_print  ("\n/* choice implementations */\n");
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name))
        continue;
      nspace.setFromSymbol(ci->name);
      String name = nspace.printableForm (ci->name);
      g_print ("\n");
      g_print ("static inline SfiChoiceValues\n");
      g_print ("%s_choice_values()\n", name.c_str());
      g_print ("{\n");
      g_print ("  static const SfiChoiceValue values[%zu] = {\n", ci->contents.size());
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
        g_print ("    { \"%s\", \"%s\" },\n", cUC_NAME (vi->name), vi->label.c_str());  // FIXME: i18n and blurb
      g_print ("  };\n");
      g_print ("  static const SfiChoiceValues choice_values = {\n");
      g_print ("    G_N_ELEMENTS (values), values,\n");
      g_print ("  };\n");
      g_print ("  return choice_values;\n");
      g_print ("}\n\n");
    }
}

void
CodeGeneratorClientCxx::printRecSeqForwardDecl (NamespaceHelper& nspace)
{
  vector<Sequence>::const_iterator si;
  vector<Record>::const_iterator ri;

  g_print ("\n/* record/sequence prototypes */\n");

  /* forward declarations for records */
  for (ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name))
        continue;

      nspace.setFromSymbol(ri->name);
      String name = nspace.printableForm (ri->name);

      g_print("\n");
      g_print("class %s;\n", name.c_str());
      g_print("typedef Sfi::RecordHandle<%s> %sHandle;\n", name.c_str(), name.c_str());
    }

  /* forward declarations for sequences */
  for (si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      nspace.setFromSymbol(si->name);
      String name = nspace.printableForm (si->name);

      g_print("\n");
      g_print("class %s;\n", name.c_str());
    }
}

void CodeGeneratorClientCxx::printRecSeqDefinition (NamespaceHelper& nspace)
{
  vector<Param>::const_iterator pi;

  g_print ("\n/* record/sequence definitions */\n");

  /* sequences */
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      nspace.setFromSymbol(si->name);

      /* FIXME: need optimized refcounted copy-on-write sequences as base types */

      String name = nspace.printableForm (si->name);
      String content = typeField (si->content.type);
      
      g_print ("\n");
      g_print ("class %s : public Sfi::Sequence<%s> {\n", name.c_str(), content.c_str());
      g_print ("public:\n");
      g_print ("  static inline %s from_seq (SfiSeq *seq);\n", cTypeRet (si->name));
      g_print ("  static inline SfiSeq *to_seq (%s seq);\n", cTypeArg (si->name));
      g_print ("  static inline %s value_get_seq (const GValue *value);\n", cTypeRet (si->name));
      g_print ("  static inline void value_set_seq (GValue *value, %s self);\n", cTypeArg (si->name));
      /* FIXME: make this private (or delete it) */
      g_print ("  static inline const char* type_name () { return \"%s\"; }\n", makeMixedName (si->name).c_str());
      g_print("};\n");
      g_print ("\n");
    }

  /* records */
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      nspace.setFromSymbol(ri->name);
      String name = nspace.printableForm (ri->name);
      String type_name = makeMixedName (ri->name).c_str();

      g_print ("\n");
      g_print ("class %s : public ::Sfi::GNewable {\n", name.c_str());
      g_print ("public:\n");
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  g_print ("  %s %s;\n", cTypeField (pi->type), pi->name.c_str());
	}
      g_print ("  static inline %s from_rec (SfiRec *rec);\n", cTypeRet(ri->name));
      g_print ("  static inline SfiRec *to_rec (%s ptr);\n", cTypeArg(ri->name));
      g_print ("  static inline %s value_get_rec (const GValue *value);\n", cTypeRet(ri->name));
      g_print ("  static inline void value_set_rec (GValue *value, %s self);\n", cTypeArg (ri->name));
      /* FIXME: make this private (or delete it) */
      g_print ("  static inline const char* type_name () { return \"%s\"; }\n", type_name.c_str());
      g_print ("};\n");
      g_print ("\n");
    }
}

void CodeGeneratorClientCxx::printRecSeqImpl (NamespaceHelper& nspace)
{
  g_print ("\n/* record/sequence implementations */\n");

  /* sequence members */
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name))
        continue;
      nspace.setFromSymbol(si->name);
      String name = nspace.printableForm (si->name);
      String nname = si->name;
      String type_name = makeMixedName (si->name).c_str();

      String elementFromValue = createTypeCode (si->content.type, "element", MODEL_FROM_VALUE);
      g_print("%s\n", cTypeRet (si->name));
      g_print("%s::from_seq (SfiSeq *sfi_seq)\n", nname.c_str());
      g_print("{\n");
      g_print("  %s seq;\n", cTypeRet (si->name));
      g_print("  guint i, length;\n");
      g_print("\n");
      g_print("  g_return_val_if_fail (sfi_seq != NULL, seq);\n");
      g_print("\n");
      g_print("  length = sfi_seq_length (sfi_seq);\n");
      g_print("  seq.resize (length);\n");
      g_print("  for (i = 0; i < length; i++)\n");
      g_print("  {\n");
      g_print("    GValue *element = sfi_seq_get (sfi_seq, i);\n");
      g_print("    seq[i] = %s;\n", elementFromValue.c_str());
      g_print("  }\n");
      g_print("  return seq;\n");
      g_print("}\n\n");

      String elementToValue = createTypeCode (si->content.type, "seq[i]", MODEL_TO_VALUE);
      g_print("SfiSeq *\n");
      g_print("%s::to_seq (%s seq)\n", nname.c_str(), cTypeArg (si->name));
      g_print("{\n");
      g_print("  SfiSeq *sfi_seq = sfi_seq_new ();\n");
      g_print("  for (guint i = 0; i < seq.length(); i++)\n");
      g_print("  {\n");
      g_print("    GValue *element = %s;\n", elementToValue.c_str());
      g_print("    sfi_seq_append (sfi_seq, element);\n");
      g_print("    sfi_value_free (element);\n");        // FIXME: couldn't we have take_append
      g_print("  }\n");
      g_print("  return sfi_seq;\n");
      g_print("}\n\n");

      g_print ("%s\n", cTypeRet (si->name));
      g_print ("%s::value_get_seq (const GValue *value)\n", nname.c_str());
      g_print ("{\n");
      g_print ("  return ::Sfi::cxx_value_get_seq< %s> (value);\n", nname.c_str());
      g_print ("}\n\n");
      g_print ("void\n");
      g_print ("%s::value_set_seq (GValue *value, %s self)\n", nname.c_str(), cTypeArg (si->name));
      g_print ("{\n");
      g_print ("  ::Sfi::cxx_value_set_seq< %s> (value, self);\n", nname.c_str());
      g_print ("}\n\n");
    }

  /* record members */
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name))
        continue;
      nspace.setFromSymbol(ri->name);
      String name = nspace.printableForm (ri->name);
      String nname = ri->name;
      String type_name = makeMixedName (ri->name).c_str();
      
      g_print("%s\n", cTypeRet (ri->name));
      g_print("%s::from_rec (SfiRec *sfi_rec)\n", nname.c_str());
      g_print("{\n");
      g_print("  GValue *element;\n");
      g_print("\n");
      g_print("  if (!sfi_rec)\n");
      g_print("    return Sfi::INIT_NULL;\n");
      g_print("\n");
      g_print("  %s rec = Sfi::INIT_DEFAULT;\n", cTypeField (ri->name));
      for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  String elementFromValue = createTypeCode (pi->type, "element", MODEL_FROM_VALUE);

	  g_print("  element = sfi_rec_get (sfi_rec, \"%s\");\n", pi->name.c_str());
	  g_print("  if (element)\n");
	  g_print("    rec->%s = %s;\n", pi->name.c_str(), elementFromValue.c_str());
	}
      g_print("  return rec;\n");
      g_print("}\n\n");

      g_print("SfiRec *\n");
      g_print("%s::to_rec (%s rec)\n", nname.c_str(), cTypeArg (ri->name));
      g_print("{\n");
      g_print("  SfiRec *sfi_rec;\n");
      g_print("  GValue *element;\n");
      g_print("\n");
      g_print("  if (!rec)\n");
      g_print("    return NULL;\n");
      g_print("\n");
      g_print("  sfi_rec = sfi_rec_new ();\n");
      for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  String elementToValue = createTypeCode (pi->type, "rec->" + pi->name, MODEL_TO_VALUE);
	  g_print("  element = %s;\n", elementToValue.c_str());
	  g_print("  sfi_rec_set (sfi_rec, \"%s\", element);\n", pi->name.c_str());
	  g_print("  sfi_value_free (element);\n");        // FIXME: couldn't we have take_set
	}
      g_print("  return sfi_rec;\n");
      g_print("}\n\n");

      g_print ("%s\n", cTypeRet(ri->name));
      g_print ("%s::value_get_rec (const GValue *value)\n", nname.c_str());
      g_print ("{\n");
      g_print ("  return ::Sfi::cxx_value_get_rec< %s> (value);\n", nname.c_str());
      g_print ("}\n\n");
      g_print ("void\n");
      g_print ("%s::value_set_rec (GValue *value, %s self)\n", nname.c_str(), cTypeArg (ri->name));
      g_print ("{\n");
      g_print ("  ::Sfi::cxx_value_set_rec< %s> (value, self);\n", nname.c_str());
      g_print ("}\n\n");
    }
}

bool CodeGeneratorClientCxx::run ()
{
  vector<Choice>::const_iterator ei;
  vector<Param>::const_iterator pi;
  vector<Class>::const_iterator ci;
  vector<Method>::const_iterator mi;
 
  g_print("\n/*-------- begin %s generated code --------*/\n\n\n", options.sfidlName.c_str());

  if (generateHeader)
    {
      /* choices */
      for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
        {
          if (parser.fromInclude (ei->name)) continue;

          nspace.setFromSymbol (ei->name);

          g_print("\nenum %s {\n", nspace.printableForm (ei->name).c_str());
          for (vector<ChoiceValue>::const_iterator ci = ei->contents.begin(); ci != ei->contents.end(); ci++)
            {
              /* don't export server side assigned choice values to the client */
              String ename = makeUpperName (nspace.printableForm (ci->name));
              g_print("  %s = %d,\n", ename.c_str(), ci->sequentialValue);
            }
          g_print("};\n");
	}
      nspace.leaveAll();

      /* choice converters */
      for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
	{
	  String name = nspace.printableForm (ei->name);
	  String lname = makeLowerName (ei->name);

	  g_print("const gchar* %s_to_choice (%s value);\n", lname.c_str(), name.c_str());
	  g_print("%s %s_from_choice (const gchar *choice);\n", name.c_str(), lname.c_str());
        }

      g_print("\n");
      /* prototypes for classes */
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;

	  nspace.setFromSymbol (ci->name);
	  String name = nspace.printableForm (ci->name);

	  g_print("class %s;\n", name.c_str());
	}

      printRecSeqForwardDecl (nspace);

      /* classes */
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;

	  nspace.setFromSymbol (ci->name);
	  String name = nspace.printableForm (ci->name);

	  String init;
	  g_print("\n");
	  if (ci->inherits == "")
	    {
	      g_print("class %s {\n", name.c_str());
	      g_print("protected:\n");
	      g_print("  SfiProxy _object_id;\n");
	      init = "_object_id";
	    }
	  else
	    {
	      g_print("class %s : public %s {\n", name.c_str(), ci->inherits.c_str());
	      init = ci->inherits;
	    }
	  g_print("public:\n");
	  g_print("  %s() : %s (0) {}\n", name.c_str(), init.c_str());
	  g_print("  %s(SfiProxy p) : %s (p) {}\n", name.c_str(), init.c_str());
	  g_print("  %s(const %s& other) : %s (other._object_id) {}\n",
	                                  name.c_str(), name.c_str(), init.c_str());
	  g_print("  SfiProxy _proxy() const { return _object_id; }\n");
	  g_print("  operator bool() const { return _object_id != 0; }\n");
	  printMethods(*ci);
	  printProperties(*ci);
	  g_print("};\n");
	}
      printRecSeqDefinition (nspace);
      printRecSeqImpl (nspace);
    }

  if (generateSource)
    {
      /* choice utils */
      printChoiceConverters();
      g_print("\n");

      /* methods */
      for (ci = parser.getClasses().begin(); ci != parser.getClasses().end(); ci++)
	{
	  if (parser.fromInclude (ci->name)) continue;
	  printMethods(*ci);
	  printProperties(*ci);
	}
    }

  g_print("\n");
  for (mi = parser.getProcedures().begin(); mi != parser.getProcedures().end(); mi++)
    {
      if (parser.fromInclude (mi->name)) continue;

      if (generateHeader)
	nspace.setFromSymbol (mi->name);
      printProcedure (*mi, generateHeader);
    }
  g_print("\n");
  nspace.leaveAll();
  g_print("\n/*-------- end %s generated code --------*/\n\n\n", options.sfidlName.c_str());

  return 1;
}

String CodeGeneratorClientCxx::makeProcName (const String& className, const String& procName)
{
  if (className == "")
    {
      return nspace.printableForm (nspace.namespaceOf (procName) + "::" +
				   makeStyleName (nspace.nameOf (procName)));
    }
  else
    {
      if (generateHeader)
	return makeStyleName (procName);
      else
	return className + "::" + makeStyleName (procName);
    }
}

void CodeGeneratorClientCxx::printMethods (const Class& cdef)
{
  vector<Method>::const_iterator mi;
  vector<Param>::const_iterator pi;
  bool proto = generateHeader;

  for (mi = cdef.methods.begin(); mi != cdef.methods.end(); mi++)
    {
      Method md;
      md.name = mi->name;
      md.result = mi->result;

      Param class_as_param;
      class_as_param.name = "_object_id";
      class_as_param.type = cdef.name;
      md.params.push_back (class_as_param);

      for(pi = mi->params.begin(); pi != mi->params.end(); pi++)
	md.params.push_back (*pi);

      if (proto) g_print ("  ");
      printProcedure (md, proto, cdef.name);
    }
}

void CodeGeneratorClientCxx::printProperties (const Class& cdef)
{
  vector<Param>::const_iterator pi;
  bool proto = generateHeader;

  for (pi = cdef.properties.begin(); pi != cdef.properties.end(); pi++)
    {
      String setProperty = makeStyleName ("set_" + pi->name);
      String getProperty = makeStyleName (pi->name);
      String newName = makeLowerName ("new_" + pi->name);
      String propName = makeLowerName (pi->name, '-');
      String ret = typeRet (pi->type);
      if (proto) {
	/* property getter */
	g_print ("  %s %s ();\n", ret.c_str(), getProperty.c_str());

	/* property setter */
	g_print ("  void %s (%s %s);\n", setProperty.c_str(), cTypeArg (pi->type), newName.c_str());
      }
      else {
	/* property getter */
	g_print ("%s\n", ret.c_str());
	g_print ("%s::%s ()\n", cdef.name.c_str(), getProperty.c_str());
	g_print ("{\n");
	g_print ("  const GValue *val;\n");
	g_print ("  val = sfi_glue_proxy_get_property (_proxy(), \"%s\");\n", propName.c_str());
	g_print ("  return %s;\n", createTypeCode (pi->type, "val", MODEL_FROM_VALUE).c_str());
	g_print ("}\n");
	g_print ("\n");
	/* property setter */
	g_print ("void\n");
	g_print ("%s::%s (%s %s)\n", cdef.name.c_str(), setProperty.c_str(),
				    cTypeArg (pi->type), newName.c_str());
	g_print ("{\n");
	String to_val = createTypeCode (pi->type, newName, MODEL_TO_VALUE).c_str();
	g_print ("  GValue *val = %s;\n", to_val.c_str());
	g_print ("  sfi_glue_proxy_set_property (_proxy(), \"%s\", val);\n", propName.c_str());
	g_print ("  sfi_value_free (val);\n");
	g_print ("}\n");
	g_print ("\n");
      }
    }
}

OptionVector
CodeGeneratorClientCxx::getOptions()
{
  OptionVector opts = CodeGeneratorCxxBase::getOptions();

  opts.push_back (make_pair ("--lower", false));
  opts.push_back (make_pair ("--mixed", false));

  return opts;
}

void
CodeGeneratorClientCxx::setOption (const String& option, const String& value)
{
  if (option == "--lower")
    {
      style = STYLE_LOWER;
    }
  else if (option == "--mixed")
    {
      style = STYLE_MIXED;
    }
  else
    {
      CodeGeneratorCxxBase::setOption (option, value);
    }
}

void
CodeGeneratorClientCxx::help()
{
  CodeGeneratorCxxBase::help();
  g_printerr (" --mixed                     mixed case identifiers (createMidiSynth)\n");
  g_printerr (" --lower                     lower case identifiers (create_midi_synth)\n");
/*
  g_printerr (stderr, " --namespace <namespace>     set the namespace to use for the code\n");
*/
}

String CodeGeneratorClientCxx::makeStyleName (const String& name)
{
  if (style == STYLE_MIXED)
    return makeLMixedName (name);
  return makeLowerName (name);
}


namespace {

class ClientCxxFactory : public Factory {
public:
  String option() const	      { return "--client-cxx"; }
  String description() const  { return "generate client C++ language binding"; }

  CodeGenerator *create (const Parser& parser) const
  {
    return new CodeGeneratorClientCxx (parser);
  }
} cxx_factory;

}
/* vim:set ts=8 sts=2 sw=2: */
