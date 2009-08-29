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
#include "sfidl-cbase.hh"
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
using std::min;
using std::max;

/*--- functions for "C and C++"-like languages ---*/

const gchar*
CodeGeneratorCBase::makeCStr (const String& str)
{
  return g_intern_string (str.c_str());
}

String
CodeGeneratorCBase::makeGTypeName (const String& name)
{
  return makeUpperName (NamespaceHelper::namespaceOf (name)
                      + "::Type" + NamespaceHelper::nameOf(name));
}

String
CodeGeneratorCBase::makeParamSpec(const Param& pdef)
{
  String pspec;
  const String group = (pdef.group != "") ? pdef.group.escaped() : "NULL";
 
  switch (parser.typeOf (pdef.type))
    {
      case CHOICE:
	{
	  pspec = "sfidl_pspec_Choice";
	  if (pdef.args == "")
	    pspec += "_default (" + group + ",\"" + pdef.name + "\",";
	  else
	    pspec += " (" + group + ", \"" + pdef.name + "\"," + pdef.args + ",";
	  pspec += makeLowerName (pdef.type) + "_get_values())";
	}
	break;
      case RECORD:
	{
	  pspec = "sfidl_pspec_Record";
	  if (pdef.args == "")
	    pspec += "_default (" + group + ", \"" + pdef.name + "\",";
	  else
	    pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + ",";
	  pspec += makeLowerName (pdef.type) + "_fields)";
	}
	break;
      case SEQUENCE:
	{
	  pspec = "sfidl_pspec_Sequence";
	  if (pdef.args == "")
	    pspec += "_default (" + group + ",\"" + pdef.name + "\",";
	  else
	    pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + ",";
	  pspec += makeLowerName (pdef.type) + "_content)";
	}
	break;
      case OBJECT:
	{
	  /* FIXME: the ParamSpec doesn't transport the type of the objects we require */
	  pspec = "sfidl_pspec_Proxy";
	  if (pdef.args == "")
	    pspec += "_default (" + group + ",\"" + pdef.name + "\")";
	  else
	    pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + ")";
	}
	break;
      default:
	{
	  pspec = "sfidl_pspec_" + pdef.pspec;
	  if (pdef.args == "")
	    pspec += "_default (" + group + ",\"" + pdef.name + "\")";
	  else
	    pspec += " (" + group + ",\"" + pdef.name + "\"," + pdef.args + ")";
	}
    }
  return pspec;
}

String CodeGeneratorCBase::scatId (SfiSCategory c)
{
  String s; s += (char) c;
  return s;
}

// how "type" looks like when passed as argument to a function
String
CodeGeneratorCBase::typeArg (const String& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:      return "void";
      case BOOL:
      case INT:
      case NUM:
      case REAL:
      case CHOICE:    return makeMixedName (type);
      case STRING:    return "const gchar*";
      case BBLOCK:    
      case FBLOCK:    
      case SFIREC:    
      case RECORD:
      case SEQUENCE:  return makeMixedName (type)+"*";
      case OBJECT:    return "SfiProxy";
    }
  return "*error*";
}

// how "type" looks like when stored as member in a struct or class
String
CodeGeneratorCBase::typeField (const String& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:      return "void";
      case BOOL:
      case INT:
      case NUM:
      case REAL:
      case CHOICE:    return makeMixedName (type);
      case STRING:    return "gchar*";
      case BBLOCK:
      case FBLOCK:
      case SFIREC:
      case RECORD:
      case SEQUENCE:  return makeMixedName (type)+"*";
      case OBJECT:    return "SfiProxy";
    }
  return "*error*";
}

// how the return type of a function returning "type" looks like
String
CodeGeneratorCBase::typeRet (const String& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:      return "void";
      case BOOL:
      case INT:
      case NUM:
      case REAL:      
      case CHOICE:    return makeMixedName (type);   
      case STRING:    return "const gchar*";
      case BBLOCK:
      case FBLOCK:
      case SFIREC:
      case RECORD:
      case SEQUENCE:  return makeMixedName (type)+"*";
      case OBJECT:    return "SfiProxy";
    }
  return "*error*";
}

// how an array of "type"s looks like ( == MODEL_MEMBER + "*" ?)
String
CodeGeneratorCBase::typeArray (const String& type)
{
  return CodeGeneratorCBase::typeField (type) + "*";
}

// how to create a new "type" called "name" (blank return value allowed)
String
CodeGeneratorCBase::funcNew (const String& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:
      case BOOL:
      case INT:
      case NUM:
      case REAL:      
      case STRING:    
      case CHOICE:    return "";
      case BBLOCK:    return "sfi_bblock_new";
      case FBLOCK:    return "sfi_fblock_new";
      case SFIREC:    return "sfi_rec_new";
      case RECORD:
      case SEQUENCE:  return makeLowerName (type)+"_new";
      case OBJECT:    return "";
    }
  return NULL;
}

String
CodeGeneratorCBase::funcCopy (const String& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:
      case BOOL:
      case INT:
      case NUM:
      case REAL:      return "";
      case STRING:    return "g_strdup";
      case CHOICE:    return "";
      case BBLOCK:    return "sfi_bblock_ref";
      case FBLOCK:    return "sfi_fblock_ref";
      case SFIREC:    return "sfi_rec_ref";
      case RECORD:
      case SEQUENCE:  return makeLowerName (type)+"_copy_shallow";
      case OBJECT:    return "";
    }
  return NULL;
}

String
CodeGeneratorCBase::funcFree (const String& type)
{
  switch (parser.typeOf (type))
    {
      case VOID:
      case BOOL:
      case INT:
      case NUM:
      case REAL:      return "";
      case STRING:    return "g_free";
      case CHOICE:    return "";
      case BBLOCK:    return "sfi_bblock_unref";
      case FBLOCK:    return "sfi_fblock_unref";
      case SFIREC:    return "sfi_rec_unref";
      case RECORD:
      case SEQUENCE:  return makeLowerName (type)+"_free";
      case OBJECT:    return "";
    }
  return NULL;
}

String CodeGeneratorCBase::createTypeCode (const String& type, TypeCodeModel model)
{
  return createTypeCode (type, "", model);
}

String CodeGeneratorCBase::createTypeCode (const String& type, const String &name,
                                           TypeCodeModel model)
{
  switch (model)
    {
      /*
       * GValues: the following models deal with getting types into and out of
       * GValues
       */
      // how to create a new "type" called "name" from a GValue*
      case MODEL_FROM_VALUE:  g_assert (name != ""); break;
      // how to convert the "type" called "name" to a GValue*
      case MODEL_TO_VALUE:    g_assert (name != ""); break;
      
      /*
       * vcall interface: the following models deal with how to perform a
       * method/procedure invocation using a given data type
       */
      // the name of the VCALL function for calling functions returning "type"
      case MODEL_VCALL:	      g_assert (name == ""); break;
      // how to pass a "type" called "name" to the VCALL function
      case MODEL_VCALL_ARG:   g_assert (name != ""); break;
      // what type a conversion results in (== MODEL_VCALL_RET ?)
      case MODEL_VCALL_CARG:  g_assert (name == ""); break;
      // how to perform the conversion of a vcall parameter called "name" (optional: "" if unused)
      case MODEL_VCALL_CONV:  g_assert (name != ""); break;
      // how to free the conversion result of "name" (optional: "" if unused)
      case MODEL_VCALL_CFREE: g_assert (name != ""); break;
      // what type a vcall result is
      case MODEL_VCALL_RET:   g_assert (name == ""); break;
      // how to convert the result of a vcall called "name" (optional: name if unused)
      case MODEL_VCALL_RCONV: g_assert (name != ""); break;
      // how to free (using GC) the result of the conversion (optional: "" if unused)
      case MODEL_VCALL_RFREE: g_assert (name != ""); break;
    }

  switch (parser.typeOf (type))
    {
      case RECORD:
      case SEQUENCE:
	{
	  if (model == MODEL_VCALL_RFREE)
	    return "if ("+name+" != NULL) sfi_glue_gc_add ("+name+", "+makeLowerName (type)+"_free)";

	  if (parser.isSequence (type))
	  {
	    if (model == MODEL_TO_VALUE)
	      return "sfi_value_new_take_seq (" + makeLowerName (type)+"_to_seq ("+name+"))";
	    if (model == MODEL_FROM_VALUE) 
	      return makeLowerName (type)+"_from_seq (sfi_value_get_seq ("+name+"))";
	    if (model == MODEL_VCALL) 
	      return "sfi_glue_vcall_seq";
	    if (model == MODEL_VCALL_ARG) 
	      return "'" + scatId (SFI_SCAT_SEQ) + "', "+name+",";
	    if (model == MODEL_VCALL_CARG) 
	      return "SfiSeq*";
	    if (model == MODEL_VCALL_CONV) 
	      return makeLowerName (type)+"_to_seq ("+name+")";
	    if (model == MODEL_VCALL_CFREE) 
	      return "sfi_seq_unref ("+name+")";
	    if (model == MODEL_VCALL_RET) 
	      return "SfiSeq*";
	    if (model == MODEL_VCALL_RCONV) 
	      return makeLowerName (type)+"_from_seq ("+name+")";
	  }
	  else
	  {
	    if (model == MODEL_TO_VALUE)   
	      return "sfi_value_new_take_rec (" + makeLowerName (type)+"_to_rec ("+name+"))";
	    if (model == MODEL_FROM_VALUE)
	      return makeLowerName (type)+"_from_rec (sfi_value_get_rec ("+name+"))";
	    if (model == MODEL_VCALL) 
	      return "sfi_glue_vcall_rec";
	    if (model == MODEL_VCALL_ARG) 
	      return "'" + scatId (SFI_SCAT_REC) + "', "+name+",";
	    if (model == MODEL_VCALL_CARG) 
	      return "SfiRec*";
	    if (model == MODEL_VCALL_CONV) 
	      return makeLowerName (type)+"_to_rec ("+name+")";
	    if (model == MODEL_VCALL_CFREE) 
	      return "sfi_rec_unref ("+name+")";
	    if (model == MODEL_VCALL_RET) 
	      return "SfiRec*";
	    if (model == MODEL_VCALL_RCONV) 
	      return makeLowerName (type)+"_from_rec ("+name+")";
	  }
	}
	break;
      case CHOICE:
	{
	  if (generateBoxedTypes)
	    {
	      if (model == MODEL_TO_VALUE)
		return "sfi_value_choice_genum ("+name+", "+makeGTypeName(type)+")";
	      if (model == MODEL_FROM_VALUE) 
		return "(" + typeField(type) + ") sfi_choice2enum (sfi_value_get_choice ("+name+"), "+makeGTypeName(type)+")";
	    }
	  else /* client code */
	    {
	      if (model == MODEL_TO_VALUE)
		return "sfi_value_choice (" + makeLowerName (type) + "_to_choice ("+name+"))";
	      if (model == MODEL_FROM_VALUE) 
		return makeLowerName (type) + "_from_choice (sfi_value_get_choice ("+name+"))";
	    }
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_choice";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_CHOICE) + "', "+makeLowerName (type)+"_to_choice ("+name+"),";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "const gchar *";
	  if (model == MODEL_VCALL_RCONV) return makeLowerName (type)+"_from_choice ("+name+")";
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      case OBJECT:
	{
	  /*
	   * FIXME: we're currently not using the type of the proxy anywhere
	   * it might for instance be worthwile being able to ensure that if
	   * we're expecting a "SfkServer" object, we will have one
	   */
	  if (model == MODEL_TO_VALUE)    return "sfi_value_proxy ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_value_get_proxy ("+name+")";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_proxy";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_PROXY) + "', "+name+",";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "SfiProxy";
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      case STRING:
	{
	  switch (model)
	    {
	      case MODEL_TO_VALUE:    return "sfi_value_string ("+name+")";
	      case MODEL_FROM_VALUE:  return "sfi_value_dup_string ("+name+")";
	      case MODEL_VCALL:       return "sfi_glue_vcall_string";
	      case MODEL_VCALL_ARG:   return "'" + scatId (SFI_SCAT_STRING) + "', "+name+",";
	      case MODEL_VCALL_CARG:  return "";
	      case MODEL_VCALL_CONV:  return "";
	      case MODEL_VCALL_CFREE: return "";
	      case MODEL_VCALL_RET:   return "const gchar*";
	      case MODEL_VCALL_RCONV: return name;
	      case MODEL_VCALL_RFREE: return "";
	    }
	}
	break;
      case BBLOCK:
	{
	  if (model == MODEL_TO_VALUE)    return "sfi_value_bblock ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_bblock_ref (sfi_value_get_bblock ("+name+"))";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_bblock";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_BBLOCK) + "', "+name+",";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "SfiBBlock*";
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      case FBLOCK:
	{
	  if (model == MODEL_TO_VALUE)    return "sfi_value_fblock ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_fblock_ref (sfi_value_get_fblock ("+name+"))";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_fblock";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_FBLOCK) + "', "+name+",";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "SfiFBlock*";
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      case SFIREC:
	{
	  /* FIXME: review this for correctness */
	  if (model == MODEL_TO_VALUE)    return "sfi_value_rec ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_rec_ref (sfi_value_get_rec ("+name+"))";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_rec";
	  if (model == MODEL_VCALL_ARG)   return "'" + scatId (SFI_SCAT_REC) + "', "+name+",";
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return "SfiRec*";
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
      default:
	{
	  /* get rid of the Sfi:: (the code wasn't written for it) */
	  String ptype = NamespaceHelper::nameOf (type);

	  String sfi = (ptype == "void") ? "" : "Sfi"; /* there is no such thing as an SfiVoid */

	  if (model == MODEL_TO_VALUE)    return "sfi_value_" + makeLowerName(ptype) + " ("+name+")";
	  if (model == MODEL_FROM_VALUE)  return "sfi_value_get_" + makeLowerName(ptype) + " ("+name+")";
	  if (model == MODEL_VCALL)       return "sfi_glue_vcall_" + makeLowerName(ptype);
	  if (model == MODEL_VCALL_ARG)
	  {
	    if (ptype == "Real")	  return "'" + scatId (SFI_SCAT_REAL) + "', "+name+",";
	    if (ptype == "Bool")	  return "'" + scatId (SFI_SCAT_BOOL) + "', "+name+",";
	    if (ptype == "Int")		  return "'" + scatId (SFI_SCAT_INT) + "', "+name+",";
	    if (ptype == "Num")		  return "'" + scatId (SFI_SCAT_NUM) + "', "+name+",";
	  }
	  if (model == MODEL_VCALL_CARG)  return "";
	  if (model == MODEL_VCALL_CONV)  return "";
	  if (model == MODEL_VCALL_CFREE) return "";
	  if (model == MODEL_VCALL_RET)   return sfi + ptype;
	  if (model == MODEL_VCALL_RCONV) return name;
	  if (model == MODEL_VCALL_RFREE) return "";
	}
	break;
    }
  return "*createTypeCode*unknown*";
}

/*--- the C language binding ---*/

String CodeGeneratorCBase::makeProcName (const String& className,
	                                 const String& procName)
{
  if (className == "")
    return makeLowerName(procName);
  else
    return makeLowerName(className) + "_" + makeLowerName(procName);
}

void CodeGeneratorCBase::printProcedure (const Method& mdef, bool proto, const String& className)
{
  vector<Param>::const_iterator pi;
  String dname, mname = makeProcName (className, mdef.name);
  
  if (className == "")
    {
      dname = makeLowerName(mdef.name, '-');
    }
  else
    {
      dname = makeMixedName(className) + "+" + makeLowerName(mdef.name, '-');
    }

  bool first = true;
  g_print ("%s%s%s (", cTypeRet (mdef.result.type), proto?" ":"\n", mname.c_str());
  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      if (pi->name == "_object_id") continue; // C++ binding: get _object_id from class

      if(!first) g_print (", ");
      first = false;
      g_print ("%s %s", cTypeArg (pi->type), pi->name.c_str());
    }
  if (first)
    g_print ("void");
  g_print (")");
  if (proto)
    {
      g_print (";\n");
      return;
    }

  g_print (" {\n");

  String vret = createTypeCode (mdef.result.type, MODEL_VCALL_RET);
  if (mdef.result.type != "void")
    g_print  ("  %s _retval;\n", vret.c_str());

  String rfree = createTypeCode (mdef.result.type, "_retval_conv", MODEL_VCALL_RFREE);
  if (rfree != "")
    g_print  ("  %s _retval_conv;\n", cTypeRet (mdef.result.type));

  map<String, String> cname;
  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      String conv = createTypeCode (pi->type, pi->name, MODEL_VCALL_CONV);
      if (conv != "")
	{
	  cname[pi->name] = pi->name + "__c";

	  String arg = createTypeCode(pi->type, MODEL_VCALL_CARG);
	  g_print ("  %s %s__c = %s;\n", arg.c_str(), pi->name.c_str(), conv.c_str());
	}
      else
	cname[pi->name] = pi->name;
    }

  g_print ("  ");
  if (mdef.result.type != "void")
    g_print ("_retval = ");
  String vcall = createTypeCode(mdef.result.type, "", MODEL_VCALL);
  g_print ("%s (\"%s\", ", vcall.c_str(), dname.c_str());

  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    g_print ("%s ", createTypeCode(pi->type, cname[pi->name], MODEL_VCALL_ARG).c_str());
  g_print ("0);\n");

  for(pi = mdef.params.begin(); pi != mdef.params.end(); pi++)
    {
      String cfree = createTypeCode (pi->type, cname[pi->name], MODEL_VCALL_CFREE);
      if (cfree != "")
	g_print ("  %s;\n", cfree.c_str());
    }

  if (mdef.result.type != "void")
    {
      String rconv = createTypeCode (mdef.result.type, "_retval", MODEL_VCALL_RCONV);

      if (rfree != "")
	{
	  g_print  ("  _retval_conv = %s;\n", rconv.c_str());
	  g_print  ("  %s;\n", rfree.c_str());
	  g_print  ("  return _retval_conv;\n");
	}
      else
	{
	  g_print  ("  return %s;\n", rconv.c_str());
	}
    }
  g_print ("}\n\n");
}

static bool choiceReverseSort(const ChoiceValue& e1, const ChoiceValue& e2)
{
  String ename1 = e1.name;
  String ename2 = e2.name;

  reverse (ename1.begin(), ename1.end());
  reverse (ename2.begin(), ename2.end());

  return ename1 < ename2;
}

void CodeGeneratorCBase::printChoiceConverters()
{
  vector<Choice>::const_iterator ei;

  for(ei = parser.getChoices().begin(); ei != parser.getChoices().end(); ei++)
    {
      if (parser.fromInclude (ei->name))
        continue;

      int minval = 1, maxval = 1;
      vector<ChoiceValue>::iterator ci;
      String name = makeLowerName (ei->name);
      String arg = typeArg (ei->name);

      /* produce reverse sorted enum array */
      vector<ChoiceValue> components = ei->contents;
      for (ci = components.begin(); ci != components.end(); ci++)
	ci->name = makeLowerName (ci->name, '-');
      sort (components.begin(), components.end(), ::choiceReverseSort);

      g_print ("static const SfiConstants %s_vals[%zd] = {\n",name.c_str(), ei->contents.size());
      for (ci = components.begin(); ci != components.end(); ci++)
	{
	  int value = ci->sequentialValue;
	  minval = min (value, minval);
	  maxval = max (value, maxval);
	  g_print ("  { \"%s\", %zd, %d },\n", ci->name.c_str(), ci->name.size(), value);
	}
      g_print ("};\n\n");

      g_print ("const gchar*\n");
      g_print ("%s_to_choice (%s value)\n", name.c_str(), arg.c_str());
      g_print ("{\n");
      g_print ("  g_return_val_if_fail (value >= %d && value <= %d, NULL);\n", minval, maxval);
      g_print ("  return sfi_constants_get_name (G_N_ELEMENTS (%s_vals), %s_vals, value);\n",
	  name.c_str(), name.c_str());
      g_print ("}\n\n");

      g_print ("%s\n", cTypeRet (ei->name));
      g_print ("%s_from_choice (const gchar *choice)\n", name.c_str());
      g_print ("{\n");
      g_print ("  return (%s) (choice ? sfi_constants_get_index (G_N_ELEMENTS (%s_vals), "
	                    "%s_vals, choice) : 0);\n", cTypeRet (ei->name), name.c_str(), name.c_str());
      g_print ("}\n");
      g_print ("\n");
    }
}

void CodeGeneratorCBase::printClientRecordPrototypes()
{
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      String mname = makeMixedName (ri->name);
      g_print ("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
    }
}

void CodeGeneratorCBase::printClientSequencePrototypes()
{
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      String mname = makeMixedName (si->name);
      g_print ("typedef struct _%s %s;\n", mname.c_str(), mname.c_str());
    }
}

void CodeGeneratorCBase::printClientRecordDefinitions()
{
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      String mname = makeMixedName (ri->name.c_str());

      g_print ("struct _%s {\n", mname.c_str());
      for (vector<Param>::const_iterator pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  g_print ("  %s %s;\n", cTypeField (pi->type), pi->name.c_str());
	}
      g_print ("};\n");
    }
  g_print ("\n");
}

void CodeGeneratorCBase::printClientSequenceDefinitions()
{
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      String mname = makeMixedName (si->name.c_str());
      String array = typeArray (si->content.type);
      String elements = si->content.name;

      g_print ("struct _%s {\n", mname.c_str());
      g_print ("  guint n_%s;\n", elements.c_str ());
      g_print ("  %s %s;\n", array.c_str(), elements.c_str());
      g_print ("};\n");
    }
}

void CodeGeneratorCBase::printClientRecordMethodPrototypes (PrefixSymbolMode mode)
{
  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      String ret = typeRet (ri->name);
      String arg = typeArg (ri->name);
      String lname = makeLowerName (ri->name.c_str());

      if (mode == generatePrefixSymbols)
	{
	  prefix_symbols.push_back (lname + "_new");
	  prefix_symbols.push_back (lname + "_copy_shallow");
	  prefix_symbols.push_back (lname + "_from_rec");
	  prefix_symbols.push_back (lname + "_to_rec");
	  prefix_symbols.push_back (lname + "_free");
	}
      else
	{
	  g_print ("%s %s_new (void);\n", ret.c_str(), lname.c_str());
	  g_print ("%s %s_copy_shallow (%s rec);\n", ret.c_str(), lname.c_str(), arg.c_str());
	  g_print ("%s %s_from_rec (SfiRec *sfi_rec);\n", ret.c_str(), lname.c_str());
	  g_print ("SfiRec *%s_to_rec (%s rec);\n", lname.c_str(), arg.c_str());
	  g_print ("void %s_free (%s rec);\n", lname.c_str(), arg.c_str());
	  g_print ("\n");
	}
    }
  g_print ("\n");
}

void CodeGeneratorCBase::printClientSequenceMethodPrototypes (PrefixSymbolMode mode)
{
  for (vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      String ret = typeRet (si->name);
      String arg = typeArg (si->name);
      String element = typeArg (si->content.type);
      String lname = makeLowerName (si->name.c_str());

      if (mode == generatePrefixSymbols)
	{
	  prefix_symbols.push_back (lname + "_new");
	  prefix_symbols.push_back (lname + "_append");
	  prefix_symbols.push_back (lname + "_copy_shallow");
	  prefix_symbols.push_back (lname + "_from_seq");
	  prefix_symbols.push_back (lname + "_to_seq");
	  prefix_symbols.push_back (lname + "_resize");
	  prefix_symbols.push_back (lname + "_free");
	}
      else
	{
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
}

void CodeGeneratorCBase::printClientRecordMethodImpl()
{
  vector<Param>::const_iterator pi;

  for (vector<Record>::const_iterator ri = parser.getRecords().begin(); ri != parser.getRecords().end(); ri++)
    {
      if (parser.fromInclude (ri->name)) continue;

      String ret = typeRet (ri->name);
      String arg = typeArg (ri->name);
      String lname = makeLowerName (ri->name.c_str());
      String mname = makeMixedName (ri->name.c_str());

      g_print ("%s\n", ret.c_str());
      g_print ("%s_new (void)\n", lname.c_str());
      g_print ("{\n");
      g_print ("  %s rec = g_new0 (%s, 1);\n", arg.c_str(), mname.c_str());
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  /* FIXME(tim): this needs to be much more versatile, so we can e.g. change
	   * whether record fields are NULL initialized (need for category->icon)
	   *
	   * FIXME(stw): probably all record fields will be NULL initialized (thats the
	   * way we do it in the C++ language binding)
	   */
	  String init = funcNew (pi->type);
	  if (init != "") g_print ("  rec->%s = %s();\n", pi->name.c_str(), init.c_str());
	}
      g_print ("  return rec;\n");
      g_print ("}\n\n");

      g_print ("%s\n", ret.c_str());
      g_print ("%s_copy_shallow (%s rec)\n", lname.c_str(), arg.c_str());
      g_print ("{\n");
      g_print ("  %s rec_copy;\n", arg.c_str());
      g_print ("  if (!rec)\n");
      g_print ("    return NULL;");
      g_print ("\n");
      g_print ("  rec_copy = g_new0 (%s, 1);\n", mname.c_str());
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  /* FIXME(tim): this needs to be more versatile, so NULL fields can be special
	   * cased before copying */
	  String copy =  funcCopy (pi->type);
	  g_print ("  rec_copy->%s = %s (rec->%s);\n", pi->name.c_str(), copy.c_str(),
	      pi->name.c_str());
	}
      g_print ("  return rec_copy;\n");
      g_print ("}\n\n");

      g_print ("%s\n", ret.c_str());
      g_print ("%s_from_rec (SfiRec *sfi_rec)\n", lname.c_str());
      g_print ("{\n");
      g_print ("  GValue *element;\n");
      g_print ("  %s rec;\n", arg.c_str());
      g_print ("  if (!sfi_rec)\n");
      g_print ("    return NULL;\n");
      g_print ("\n");
      g_print ("  rec = g_new0 (%s, 1);\n", mname.c_str());
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  String elementFromValue = createTypeCode (pi->type, "element", MODEL_FROM_VALUE);
	  String init = funcNew (pi->type);

	  g_print ("  element = sfi_rec_get (sfi_rec, \"%s\");\n", pi->name.c_str());
	  g_print ("  if (element)\n");
	  g_print ("    rec->%s = %s;\n", pi->name.c_str(), elementFromValue.c_str());

	  if (init != "")
	    {
	      g_print ("  else\n");
	      g_print ("    rec->%s = %s();\n", pi->name.c_str(), init.c_str());
	    }
	}
      g_print ("  return rec;\n");
      g_print ("}\n\n");

      g_print ("SfiRec *\n");
      g_print ("%s_to_rec (%s rec)\n", lname.c_str(), arg.c_str());
      g_print ("{\n");
      g_print ("  SfiRec *sfi_rec;\n");
      g_print ("  GValue *element;\n");
      g_print ("  if (!rec)\n");
      g_print ("    return NULL;\n");
      g_print ("\n");
      g_print ("  sfi_rec = sfi_rec_new ();\n");
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  String elementToValue = createTypeCode (pi->type, "rec->" + pi->name, MODEL_TO_VALUE);
	  g_print ("  element = %s;\n", elementToValue.c_str());
	  g_print ("  sfi_rec_set (sfi_rec, \"%s\", element);\n", pi->name.c_str());
	  g_print ("  sfi_value_free (element);\n");        // FIXME: couldn't we have take_set
	}
      g_print ("  return sfi_rec;\n");
      g_print ("}\n\n");

      g_print ("void\n");
      g_print ("%s_free (%s rec)\n", lname.c_str(), arg.c_str());
      g_print ("{\n");
      g_print ("  g_return_if_fail (rec != NULL);\n");
      /* FIXME (tim): should free functions generally demand non-NULL structures? */
      g_print ("  \n");
      for (pi = ri->contents.begin(); pi != ri->contents.end(); pi++)
	{
	  /* FIXME (tim): needs to be more verstaile, so NULL fields can be properly special cased */
	  // FIXME (stw): there _should_ be no NULL fields in some cases (sequences)!
	  String free = funcFree (pi->type);
	  if (free != "") g_print ("  if (rec->%s) %s (rec->%s);\n",
	      pi->name.c_str(), free.c_str(), pi->name.c_str());
	}
      g_print ("  g_free (rec);\n");
      g_print ("}\n\n");
      g_print ("\n");
    }
}

void CodeGeneratorCBase::printClientSequenceMethodImpl()
{
  for(vector<Sequence>::const_iterator si = parser.getSequences().begin(); si != parser.getSequences().end(); si++)
    {
      if (parser.fromInclude (si->name)) continue;

      String ret = typeRet (si->name);
      String arg = typeArg (si->name);
      String element = typeArg (si->content.type);
      String elements = si->content.name;
      String lname = makeLowerName (si->name.c_str());
      String mname = makeMixedName (si->name.c_str());

      g_print ("%s\n", ret.c_str());
      g_print ("%s_new (void)\n", lname.c_str());
      g_print ("{\n");
      g_print ("  return g_new0 (%s, 1);\n",mname.c_str());
      g_print ("}\n\n");

      String elementCopy = funcCopy (si->content.type);
      g_print ("void\n");
      g_print ("%s_append (%s seq, %s element)\n", lname.c_str(), arg.c_str(), element.c_str());
      g_print ("{\n");
      g_print ("  g_return_if_fail (seq != NULL);\n");
      g_print ("\n");
      g_print ("  seq->%s = g_realloc (seq->%s, "
	  "(seq->n_%s + 1) * sizeof (seq->%s[0]));\n",
	  elements.c_str(), elements.c_str(), elements.c_str(), elements.c_str());
      g_print ("  seq->%s[seq->n_%s++] = %s (element);\n", elements.c_str(), elements.c_str(),
	  elementCopy.c_str());
      g_print ("}\n\n");

      g_print ("%s\n", ret.c_str());
      g_print ("%s_copy_shallow (%s seq)\n", lname.c_str(), arg.c_str());
      g_print ("{\n");
      g_print ("  %s seq_copy;\n", arg.c_str ());
      g_print ("  guint i;\n");
      g_print ("  if (!seq)\n");
      g_print ("    return NULL;\n");
      g_print ("\n");
      g_print ("  seq_copy = %s_new ();\n", lname.c_str());
      g_print ("  for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
      g_print ("    %s_append (seq_copy, seq->%s[i]);\n", lname.c_str(), elements.c_str());
      g_print ("  return seq_copy;\n");
      g_print ("}\n\n");

      String elementFromValue = createTypeCode (si->content.type, "element", MODEL_FROM_VALUE);
      g_print ("%s\n", ret.c_str());
      g_print ("%s_from_seq (SfiSeq *sfi_seq)\n", lname.c_str());
      g_print ("{\n");
      g_print ("  %s seq;\n", arg.c_str());
      g_print ("  guint i, length;\n");
      g_print ("\n");
      g_print ("  g_return_val_if_fail (sfi_seq != NULL, NULL);\n");
      g_print ("\n");
      g_print ("  length = sfi_seq_length (sfi_seq);\n");
      g_print ("  seq = g_new0 (%s, 1);\n",mname.c_str());
      g_print ("  seq->n_%s = length;\n", elements.c_str());
      g_print ("  seq->%s = g_malloc (seq->n_%s * sizeof (seq->%s[0]));\n\n",
	  elements.c_str(), elements.c_str(), elements.c_str());
      g_print ("  for (i = 0; i < length; i++)\n");
      g_print ("    {\n");
      g_print ("      GValue *element = sfi_seq_get (sfi_seq, i);\n");
      g_print ("      seq->%s[i] = %s;\n", elements.c_str(), elementFromValue.c_str());
      g_print ("    }\n");
      g_print ("  return seq;\n");
      g_print ("}\n\n");

      String elementToValue = createTypeCode (si->content.type, "seq->" + elements + "[i]", MODEL_TO_VALUE);
      g_print ("SfiSeq *\n");
      g_print ("%s_to_seq (%s seq)\n", lname.c_str(), arg.c_str());
      g_print ("{\n");
      g_print ("  SfiSeq *sfi_seq;\n");
      g_print ("  guint i;\n");
      g_print ("\n");
      g_print ("  g_return_val_if_fail (seq != NULL, NULL);\n");
      g_print ("\n");
      g_print ("  sfi_seq = sfi_seq_new ();\n");
      g_print ("  for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
      g_print ("    {\n");
      g_print ("      GValue *element = %s;\n", elementToValue.c_str());
      g_print ("      sfi_seq_append (sfi_seq, element);\n");
      g_print ("      sfi_value_free (element);\n");        // FIXME: couldn't we have take_append
      g_print ("    }\n");
      g_print ("  return sfi_seq;\n");
      g_print ("}\n\n");

      // FIXME: we should check whether we _really_ need to deal with a seperate free_check
      //        function here, as it needs to be specialcased everywhere
      //
      //        especially in some cases (sequences of sequences) we will free invalid
      //        data structures without complaining!
      String element_i_free_check = "if (seq->" + elements + "[i]) ";
      String element_i_free = funcFree (si->content.type);
      String element_i_new = funcNew (si->content.type);
      g_print ("void\n");
      g_print ("%s_resize (%s seq, guint new_size)\n", lname.c_str(), arg.c_str());
      g_print ("{\n");
      g_print ("  g_return_if_fail (seq != NULL);\n");
      g_print ("\n");
      if (element_i_free != "")
	{
	  g_print ("  if (seq->n_%s > new_size)\n", elements.c_str());
	  g_print ("    {\n");
	  g_print ("      guint i;\n");
	  g_print ("      for (i = new_size; i < seq->n_%s; i++)\n", elements.c_str());
	  g_print ("        %s %s (seq->%s[i]);\n", element_i_free_check.c_str(),
	      element_i_free.c_str(), elements.c_str());
	  g_print ("    }\n");
	}
      g_print ("\n");
      g_print ("  seq->%s = g_realloc (seq->%s, new_size * sizeof (seq->%s[0]));\n",
	  elements.c_str(), elements.c_str(), elements.c_str());
      g_print ("  if (new_size > seq->n_%s)\n", elements.c_str());
      if (element_i_new != "")
	{
	  g_print ("    {\n");
	  g_print ("      guint i;\n");
	  g_print ("      for (i = seq->n_%s; i < new_size; i++)\n", elements.c_str());
	  g_print ("        seq->%s[i] = %s();\n", elements.c_str(), element_i_new.c_str());
	  g_print ("    }\n");
	}
      else
	{
	  g_print ("    memset (&seq->%s[seq->n_%s], 0, sizeof(seq->%s[0]) * (new_size - seq->n_%s));\n",
	      elements.c_str(), elements.c_str(), elements.c_str(), elements.c_str());
	}
      g_print ("  seq->n_%s = new_size;\n", elements.c_str());
      g_print ("}\n\n");

      g_print ("void\n");
      g_print ("%s_free (%s seq)\n", lname.c_str(), arg.c_str());
      g_print ("{\n");
      if (element_i_free != "")
	g_print ("  guint i;\n\n");
      g_print ("  g_return_if_fail (seq != NULL);\n");
      g_print ("  \n");
      if (element_i_free != "")
	{
	  g_print ("  for (i = 0; i < seq->n_%s; i++)\n", elements.c_str());
	  g_print ("        %s %s (seq->%s[i]);\n", element_i_free_check.c_str(),
	      element_i_free.c_str(), elements.c_str());
	}
      g_print ("  g_free (seq->%s);\n", elements.c_str());
      g_print ("  g_free (seq);\n");
      g_print ("}\n\n");
      g_print ("\n");
    }
}

void CodeGeneratorCBase::printClientChoiceDefinitions()
{
  for(vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name)) continue;

      String mname = makeMixedName (ci->name);
      String lname = makeLowerName (ci->name);
      g_print ("\ntypedef enum {\n");
      for (vector<ChoiceValue>::const_iterator vi = ci->contents.begin(); vi != ci->contents.end(); vi++)
	{
	  /* don't export server side assigned choice values to the client */
	  String ename = makeUpperName (vi->name);
	  g_print ("  %s = %d,\n", ename.c_str(), vi->sequentialValue);
	}
      g_print ("} %s;\n", mname.c_str());
    }
  g_print ("\n");
}

void CodeGeneratorCBase::printClientChoiceConverterPrototypes (PrefixSymbolMode mode)
{
  for (vector<Choice>::const_iterator ci = parser.getChoices().begin(); ci != parser.getChoices().end(); ci++)
    {
      if (parser.fromInclude (ci->name)) continue;

      String mname = makeMixedName (ci->name);
      String lname = makeLowerName (ci->name);

      if (mode == generatePrefixSymbols)
	{
	  prefix_symbols.push_back (lname + "_to_choice");
	  prefix_symbols.push_back (lname + "_from_choice");
	}
      else
	{
	  g_print  ("const gchar* %s_to_choice (%s value);\n", lname.c_str(), mname.c_str());
	  g_print  ("%s %s_from_choice (const gchar *choice);\n", mname.c_str(), lname.c_str());
	}
    }
  g_print ("\n");
}


/* vim:set ts=8 sts=2 sw=2: */
