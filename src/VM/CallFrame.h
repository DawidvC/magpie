#pragma once

#include "Array.h"
#include "Chunk.h"
#include "GC.h"
#include "Macros.h"
#include "Managed.h"
#include "Multimethod.h"
#include "Object.h"

namespace magpie
{
  class CallFrame : public Managed
  {
  public:
    CallFrame(gc<Chunk> chunk);

    // TODO(bob): Implement reach.

    int  getInstruction()                { return instruction_; }
    void setInstruction(int instruction) { instruction_ = instruction; }

    gc<Chunk>       getChunk()           { return chunk_; }
    gc<Object>      getRegister(int index) { return registers_[index]; }

    void setRegister(int index, gc<Object> value) { registers_[index] = value; }

  private:
    gc<Chunk>               chunk_;
    int                     instruction_;
    Array<gc<Object> >      registers_;

    NO_COPY(CallFrame);
  };
}

