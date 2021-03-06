/* BseEvaluator - evaluates expressions
 * Copyright (C) 2003 Stefan Westerfeld <stefan@space.twc.de>
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
#include "bseevaluator.genidl.hh"
#include "symbols.hh"
#include "instruction.hh"
#include "cpu.hh"
#include "compiler.hh"

#include <math.h>
#include <string.h>

namespace Bse {
using namespace std;
using namespace EvaluatorUtils;

class Evaluator : public EvaluatorBase
{
  int input_reg, output_reg;
  vector<Instruction> instructions;

  struct Properties : public EvaluatorProperties {
    vector<Instruction> instructions;
    int input_reg, output_reg;

    explicit Properties (Evaluator *e) : EvaluatorProperties (e)
    {
      instructions = e->instructions;
      input_reg = e->input_reg;
      output_reg = e->output_reg;
    }
  };

  class Module : public SynthesisModule {
    CPU cpu;
    int input_reg, output_reg;
  public:
    void
    reset()
    {
    }
    void
    config(Properties *params)
    {
      cpu.set_program (params->instructions);
      input_reg = params->input_reg;
      output_reg = params->output_reg;
    }
    void
    process(unsigned int samples)
    {
      const float *input = istream (ICHANNEL_INPUT).values;
      float *output = ostream (OCHANNEL_OUTPUT).values;
     
      cpu.execute_1_1_block (input_reg, output_reg, input, output, samples);
    }
  };
public:
  void
  set_status(const Sfi::String& new_status)
  {
    status = new_status;
    notify ("status");
  }
  bool
  property_changed (EvaluatorPropertyID prop_id)
  {
    switch (prop_id)
      {
      case PROP_SOURCE:
        {
          vector<char>  source_vec (source.c_str(), source.c_str() + source.length());
          vector<Token> tokens;
          vector<Instruction> new_instructions;
          Symbols symbols;
          string error;
          
          input_reg = symbols.alloc("input");
          output_reg = symbols.alloc("output");
          
          error = Compiler::tokenize (symbols, source_vec, tokens);
          if (error != "")
            {
              set_status("ERROR: " + error);
              break;
            }
          
          error = Compiler::compile (symbols, tokens, new_instructions);
          if (error != "")
            {
              set_status("ERROR: " + error);
              break;
            }
          
          instructions = new_instructions;
          
          CPU cpu;
          cpu.set_program(instructions);
          cpu.print_program(symbols);
          
          set_status("compile ok.");
        }
        break;
      default: ;
      }
    return false;
  }
  BSE_EFFECT_INTEGRATE_MODULE (Evaluator, Module, Properties);

};

BSE_CXX_DEFINE_EXPORTS();
BSE_CXX_REGISTER_EFFECT (Evaluator);

} // Bse

/* vim:set ts=8 sw=2 sts=2: */
