#include "Compiler/Compiler.h"
#include "Memory/Memory.h"
#include "Platform/Environment.h"
#include "Syntax/Parser.h"
#include "VM/Method.h"
#include "VM/Module.h"
#include "VM/Object.h"
#include "VM/VM.h"

namespace magpie
{
  bool Module::parse(ErrorReporter& reporter)
  {
    ASSERT(ast_.isNull(), "Module is already parsed.");

    gc<String> code = readFile(path_);
    if (code.isNull())
    {
      reporter.error(NULL, "Could not read \"%s\".", path_->cString());
      return false;
    }

    source_ = new SourceFile(path_, code);

    Parser parser(source_, reporter);
    ast_ = parser.parseModule();

    return !ast_.isNull();
  }

  void Module::addImports(VM& vm, ErrorReporter& reporter)
  {
    // Implicitly import core (unless we are core).
    if (*name_ != "core")
    {
      vm.importModule(reporter, this, NULL, String::create("core"));
    }

    // Load all of the imports.
    for (int i = 0; i < ast_->body()->expressions().count(); i++)
    {
      ImportExpr* import = ast_->body()->expressions()[i]->asImportExpr();
      if (import == NULL) continue;

      vm.importModule(reporter, this, import->pos(), import->name());
    }
  }

  bool Module::compile(VM& vm)
  {
    ASSERT(!ast_.isNull(), "Must parse module before compiling.");

    ErrorReporter reporter;
    Compiler::compileModule(vm, reporter, ast_, this);

    // Now that we've compiled it, we can throw away the AST.
    ast_ = NULL;

    return reporter.numErrors() == 0;
  }

  void Module::setBody(gc<Chunk> body)
  {
    body_ = body;
  }

  void Module::addVariable(gc<String> name, gc<Object> value)
  {
    variableNames_.add(name);
    variables_.add(value);
  }

  int Module::findVariable(gc<String> name)
  {
    for (int i = 0; i < variableNames_.count(); i++)
    {
      if (variableNames_[i] == name) return i;
    }

    return -1;
  }

  gc<Object> Module::getVariable(const char* name) const
  {
    for (int i = 0; i < variableNames_.count(); i++)
    {
      if (*variableNames_[i] == name) return variables_[i];
    }

    ASSERT(false, "Could not find expected variable.");
    return NULL;
  }

  void Module::setVariable(int index, gc<Object> value)
  {
    variables_[index] = value;
  }

  void Module::reach()
  {
    name_.reach();
    path_.reach();
    ast_.reach();
    body_.reach();
    variables_.reach();
    variableNames_.reach();
  }
}

