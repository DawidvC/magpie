#include "Syntax/Ast.h"

namespace magpie
{
  void ClassField::reach()
  {
    name_.reach();
    pattern_.reach();
    initializer_.reach();
  }

  void AndExpr::trace(std::ostream& out) const
  {
    out << "(and " << left_ << " " << right_ << ")";
  }

  void AssignExpr::trace(std::ostream& out) const
  {
    out << "(" << lvalue_ << " = " << value_ << ")";
  }

  void AsyncExpr::trace(std::ostream& out) const
  {
    out << "(async " << body_ << ")";
  }

  void AtomExpr::trace(std::ostream& out) const
  {
    switch (atom_)
    {
      case ATOM_FALSE: out << "false"; break;
      case ATOM_TRUE: out << "true"; break;
      case ATOM_NOTHING: out << "nothing"; break;
      case ATOM_DONE: out << "done"; break;
      case ATOM_NO_METHOD: out << "<no method>"; break;
    }
  }

  void BreakExpr::trace(std::ostream& out) const
  {
    out << "(break)";
  }

  void CallExpr::trace(std::ostream& out) const
  {
    out << "(";
    if (!leftArg_.isNull())
    {
      out << leftArg_ << " ";
    }

    out << name_;

    if (!rightArg_.isNull())
    {
      out << " " << rightArg_;
    }

    out << ")";
  }

  void CatchExpr::trace(std::ostream& out) const
  {
    out << "(try " << body_;

    for (int i = 0; i < catches_.count(); i++)
    {
      out << " (catch " << catches_[i].pattern() << " then ";
      out << catches_[i].body() << ")";
    }

    out << ")";
  }

  void CharacterExpr::trace(std::ostream& out) const
  {
    // TODO(bob): Convert back to character representation.
    out << "'" << value_ << "'";
  }

  void DefExpr::trace(std::ostream& out) const
  {
    out << "(def ";
    if (!leftParam_.isNull()) out << leftParam_ << " ";
    out << name_;
    if (!rightParam_.isNull()) out << " " << rightParam_;
    if (!value_.isNull()) out << "=" << value_;

    out << " -> " << body_ << ")";
  }

  void DefClassExpr::trace(std::ostream& out) const
  {
    out << "(defclass " << name_;

    if (isNative_)
    {
      out << " native)";
      return;
    }

    out << "\n";

    for (int i = 0; i < fields_.count(); i++)
    {
      ClassField& field = *fields_[i];
      out << "  ";
      out << (field.isMutable() ? "var " : "val ") << " " << field.name();

      if (!field.pattern().isNull())
      {
        out << " " << field.pattern();
      }

      if (!field.initializer().isNull())
      {
        out << " = " << field.initializer();
      }
      out << "\n";
    }

    out << ")";
  }

  void DoExpr::trace(std::ostream& out) const
  {
    out << "(do " << body_ << ")";
  }

  void FloatExpr::trace(std::ostream& out) const
  {
    out << value_;
  }

  void FnExpr::trace(std::ostream& out) const
  {
    out << "(fn " << pattern_ << " -> " << body_ << ")";
  }

  void ForExpr::trace(std::ostream& out) const
  {
    out << "(for " << pattern_ << " in " << iterator_
        << " do " << body_ << ")";
  }

  void GetFieldExpr::trace(std::ostream& out) const
  {
    out << "(get-field " << index_ << ")";
  }

  void IfExpr::trace(std::ostream& out) const
  {
    out << "(if " << condition_ << " then " << thenArm_;

    if (elseArm_.isNull())
    {
      out << ")";
    }
    else
    {
      out << " else " << elseArm_ << ")";
    }
  }

  void ImportExpr::trace(std::ostream& out) const
  {
    out << "(import " << name_ << ")";
  }

  void IntExpr::trace(std::ostream& out) const
  {
    out << value_;
  }

  void IsExpr::trace(std::ostream& out) const
  {
    out << "(" << value_ << " is " << type_ << ")";
  }

  void ListExpr::trace(std::ostream& out) const
  {
    out << "[\n";

    for (int i = 0; i < elements_.count(); i++)
    {
      out << elements_[i];
      if (i < elements_.count() - 1) out << ",";
      out << "\n";
    }

    out << "]";
  }

  void MatchExpr::trace(std::ostream& out) const
  {
    out << "(match " << value_;

    for (int i = 0; i < cases_.count(); i++)
    {
      out << " (case " << cases_[i].pattern() << " then ";
      out << cases_[i].body() << ")";
    }

    out << ")";
  }

  void NameExpr::trace(std::ostream& out) const
  {
    out << name_;
  }

  void NativeExpr::trace(std::ostream& out) const
  {
    out << "(native " << name_ << ")";
  }

  void NotExpr::trace(std::ostream& out) const
  {
    out << "(not " << value_ << ")";
  }

  void OrExpr::trace(std::ostream& out) const
  {
    out << "(or " << left_ << " " << right_ << ")";
  }

  void RecordExpr::trace(std::ostream& out) const
  {
    out << "(";

    for (int i = 0; i < fields_.count(); i++)
    {
      if (i > 0) out << ", ";
      out << fields_[i].name << ": " << fields_[i].value;
    }

    out << ")";
  }

  void ReturnExpr::trace(std::ostream& out) const
  {
    if (value_.isNull())
    {
      out << "(return)";
    }
    else
    {
      out << "(return " << value_ << ")";
    }
  }

  void SequenceExpr::trace(std::ostream& out) const
  {
    out << "(\n";

    for (int i = 0; i < expressions_.count(); i++)
    {
      out << expressions_[i] << "\n";
    }

    out << ")";
  }

  void SetFieldExpr::trace(std::ostream& out) const
  {
    out << "(set-field " << index_ << ")";
  }

  void StringExpr::trace(std::ostream& out) const
  {
    out << "\"" << value_ << "\"";
  }

  void ThrowExpr::trace(std::ostream& out) const
  {
    out << "(throw " << value_ << ")";
  }

  void VariableExpr::trace(std::ostream& out) const
  {
    out << "(" << (isMutable_ ? "var " : "val ");
    out << pattern_ << " = " << value_ << ")";
  }

  void WhileExpr::trace(std::ostream& out) const
  {
    out << "(while " << condition_ << " " << body_ << ")";
  }

  void CallLValue::trace(std::ostream& out) const
  {
    out << call_;
  }

  void NameLValue::trace(std::ostream& out) const
  {
    out << name_;
  }

  void RecordLValue::trace(std::ostream& out) const
  {
    out << "(";

    for (int i = 0; i < fields_.count(); i++)
    {
      if (i > 0) out << ", ";
      out << fields_[i].name << ": " << fields_[i].value;
    }

    out << ")";
  }

  void WildcardLValue::trace(std::ostream& out) const
  {
    out << "_";
  }

  void RecordPattern::trace(std::ostream& out) const
  {
    out << "(";

    for (int i = 0; i < fields_.count(); i++)
    {
      if (i > 0) out << ", ";
      out << fields_[i].name << ": " << fields_[i].value;
    }

    out << ")";
  }

  void TypePattern::trace(std::ostream& out) const
  {
    out << "(is " << type_ << ")";
  }

  void ValuePattern::trace(std::ostream& out) const
  {
    out << "(== " << value_ << ")";
  }

  void VariablePattern::trace(std::ostream& out) const
  {
    if (!pattern_.isNull())
    {
      out << "(" << name_ << " " << pattern_ << ")";
    }
    else
    {
      out << name_;
    }
  }
}

