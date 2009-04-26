﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Magpie.Compilation
{
    public class TupleExpr<TExpr>
    {
        public readonly List<TExpr> Fields = new List<TExpr>();

        public TupleExpr(IEnumerable<TExpr> fields)
        {
            Fields.AddRange(fields);
        }

        public override string ToString()
        {
            return "(" + Fields.JoinAll(", ") + ")";
        }
    }

    public class TupleExpr : TupleExpr<IUnboundExpr>, IUnboundExpr
    {
        public TupleExpr(IEnumerable<IUnboundExpr> fields) : base(fields) { }

        public TReturn Accept<TReturn>(IUnboundExprVisitor<TReturn> visitor)
        {
            return visitor.Visit(this);
        }
    }

    public class BoundTupleExpr : TupleExpr<IBoundExpr>, IBoundExpr
    {
        public BoundTupleExpr(IEnumerable<IBoundExpr> fields) : base(fields) { }

        public Decl Type
        {
            get
            {
                return new TupleType(Fields.ConvertAll(field => field.Type));
            }
        }

        public TReturn Accept<TReturn>(IBoundExprVisitor<TReturn> visitor)
        {
            return visitor.Visit(this);
        }
    }
}
