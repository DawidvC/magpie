#include "VM/Fiber.h"

#include "VM/Method.h"
#include "VM/Module.h"
#include "VM/Object.h"
#include "VM/VM.h"

namespace magpie
{
  int Fiber::nextId_ = 0;

  Fiber::Fiber(VM& vm, Scheduler& scheduler, gc<FunctionObject> function,
               gc<Fiber> successor)
  : vm_(vm),
    scheduler_(scheduler),
    successor_(successor),
    isMain_(false),
    id_(nextId_++),
    stack_(),
    callFrames_(),
    nearestCatch_()
  {
    call(function, 0);
  }

  bool Fiber::isDone()
  {
    return callFrames_.count() == 0;
  }

  FiberResult Fiber::run(gc<Object>& result)
  {
    while (true)
    {
      if (Memory::checkCollect()) return FIBER_DID_GC;

      CallFrame& frame = callFrames_[-1];
      Chunk& chunk = *frame.function->chunk();
      instruction ins = chunk.code()[frame.ip++];
      OpCode op = GET_OP(ins);

      switch (op)
      {
        case OP_MOVE:
        {
          int from = GET_A(ins);
          int to = GET_B(ins);
          store(frame, to, load(frame, from));
          break;
        }

        case OP_CONSTANT:
        {
          int index = GET_A(ins);
          int slot = GET_B(ins);
          store(frame, slot, chunk.getConstant(index));
          break;
        }

        case OP_ATOM:
        {
          Atom atom = static_cast<Atom>(GET_A(ins));
          int slot = GET_B(ins);
          store(frame, slot, vm_.getAtom(atom));
          break;
        }

        case OP_METHOD:
        {
          // Adds a method to a multimethod. A is the index of the multimethod to
          // specialize. B is the index of the method to add.
          int multimethod = GET_A(ins);
          int method = GET_B(ins);
          vm_.defineMethod(multimethod, method);
          break;
        }

        case OP_RECORD:
        {
          int firstSlot = GET_A(ins);
          gc<RecordType> type = vm_.getRecordType(GET_B(ins));
          gc<Object> record = RecordObject::create(type, stack_,
              frame.stackStart + firstSlot);
          store(frame, GET_C(ins), record);
          break;
        }

        case OP_LIST:
        {
          int firstSlot = GET_A(ins);
          int numElements = GET_B(ins);

          gc<ListObject> list = new ListObject(numElements);
          for (int i = 0; i < numElements; i++)
          {
            gc<Object> element = load(frame, firstSlot + i);
            list->elements().add(element);
          }
          store(frame, GET_C(ins), list);
          break;
        }

        case OP_FUNCTION:
        {
          gc<FunctionObject> function = loadFunction(frame, GET_A(ins));
          store(frame, GET_B(ins), function);
          break;
        }

        case OP_ASYNC:
        {
          // Create a function to store the chunk and upvars.
          gc<FunctionObject> function = loadFunction(frame, GET_A(ins));
          scheduler_.spawn(function);
          break;
        }

        case OP_CLASS:
        {
          // A class definition is two instructions long.
          instruction ins2 = chunk.code()[frame.ip++];
          ASSERT(GET_OP(ins2) == OP_MOVE,
                 "Expect pseudo-instruction after OP_CLASS.");

          gc<String> name = vm_.getSymbol(GET_A(ins));

          int superclassSlot = GET_A(ins2);
          int numSuperclasses = GET_B(ins2);

          ArrayView<gc<Object> > superclasses(stack_,
              frame.stackStart + superclassSlot);
          gc<ClassObject> classObj = ClassObject::create(
              name, GET_B(ins), numSuperclasses, superclasses);

          store(frame, GET_C(ins), classObj);
          break;
        }

        case OP_GET_FIELD:
        {
          bool success = false;
          RecordObject* record = load(frame, GET_A(ins))->toRecord();

          // We can't pull record fields out of something that isn't a record.
          // TODO(bob): Should you be able to destructure arbitrary objects by
          // invoking getters with the right name?
          if (record != NULL)
          {
            int symbol = GET_B(ins);
            gc<Object> field = record->getField(symbol);

            // If the record has the field, store it.
            if (!field.isNull())
            {
              store(frame, GET_C(ins), field);
              success = true;
            }
          }

          if (!success)
          {
            gc<Object> error = DynamicObject::create(
                vm_.getClass(CLASS_NO_MATCH_ERROR));
            if (!throwError(error)) return FIBER_UNCAUGHT_ERROR;
          }
          break;
        }

        case OP_TEST_FIELD:
        {
          RecordObject* record = load(frame, GET_A(ins))->toRecord();

          // The next instruction is a pseudo-instruction containing the offset
          // to jump to.
          instruction jump = chunk.code()[frame.ip++];
          ASSERT(GET_OP(jump) == OP_JUMP,
                 "Pseudo-instruction after OP_TEST_FIELD must be OP_JUMP.");

          // We can't pull record fields out of something that isn't a record.
          // TODO(bob): Should you be able to destructure arbitrary objects by
          // invoking getters with the right name?
          bool success = false;
          if (record != NULL)
          {
            int symbol = GET_B(ins);
            gc<Object> field = record->getField(symbol);

            // If the record has the field, store it.
            if (!field.isNull())
            {
              store(frame, GET_C(ins), field);
              success = true;
            }
          }

          // Jump if the match failed.
          if (!success)
          {
            int offset = GET_B(jump);
            frame.ip += offset;
          }
          break;
        }

        case OP_GET_CLASS_FIELD:
        {
          // This assumes a certain slot layout because this opcode only
          // appears in auto-generated getter methods.
          int fieldIndex = GET_A(ins);

          gc<DynamicObject> object = asDynamic(load(frame, 0));
          store(frame, 1, object->getField(fieldIndex));
          break;
        }

        case OP_SET_CLASS_FIELD:
        {
          // This assumes a certain slot layout because this opcode only
          // appears in auto-generated getter methods.
          int fieldIndex = GET_A(ins);

          gc<DynamicObject> object = asDynamic(load(frame, 0));
          object->setField(fieldIndex, load(frame, 1));
          store(frame, 2, load(frame, 1));
          break;
        }

        case OP_GET_VAR:
        {
          int moduleIndex = GET_A(ins);
          int variableIndex = GET_B(ins);
          Module* module = vm_.getModule(moduleIndex);
          gc<Object> object = module->getVariable(variableIndex);

          if (object.isNull())
          {
            gc<Object> error = DynamicObject::create(
                vm_.getClass(CLASS_UNDEFINED_VAR_ERROR));

            if (!throwError(error)) return FIBER_UNCAUGHT_ERROR;
          }

          store(frame, GET_C(ins), object);
          break;
        }

        case OP_SET_VAR:
        {
          int moduleIndex = GET_A(ins);
          int variableIndex = GET_B(ins);
          Module* module = vm_.getModule(moduleIndex);
          gc<Object> value = load(frame, GET_C(ins));
          module->setVariable(variableIndex, value);
          break;
        }

        case OP_GET_UPVAR:
        {
          gc<Upvar> upvar = frame.function->getUpvar(GET_A(ins));
          gc<Object> object = upvar->value();
          store(frame, GET_B(ins), object);
          break;
        }

        case OP_SET_UPVAR:
        {
          gc<Upvar> upvar;
          if (GET_C(ins) == 1)
          {
            upvar = new Upvar();
            frame.function->setUpvar(GET_A(ins), upvar);
          }
          else
          {
            upvar = frame.function->getUpvar(GET_A(ins));
          }
          gc<Object> object = load(frame, GET_B(ins));
          upvar->setValue(object);
          break;
        }

        case OP_EQUAL:
        {
          gc<Object> a = load(frame, GET_A(ins));
          gc<Object> b = load(frame, GET_B(ins));
          store(frame, GET_C(ins), vm_.getBool(a->equals(b)));
          break;
        }

        case OP_NOT:
        {
          gc<Object> value = load(frame, GET_A(ins));

          // TODO(bob): Handle user-defined types.
          bool result = !value->toBool();
          store(frame, GET_A(ins), vm_.getBool(result));
          break;
        }

        case OP_IS:
        {
          gc<Object> value = load(frame, GET_A(ins));

          // TODO(bob): Handle it not being a class.
          gc<ClassObject> expected = asClass(load(frame, GET_B(ins)));
          gc<ClassObject> classObject = value->getClass(vm_);
          store(frame, GET_C(ins), vm_.getBool(classObject->is(*expected)));
          break;
        }

        case OP_JUMP:
        {
          int forward = GET_A(ins);
          int offset = GET_B(ins);
          frame.ip += (forward == 1) ? offset : -offset;
          break;
        }

        case OP_JUMP_IF_FALSE:
        {
          gc<Object> a = load(frame, GET_A(ins));
          if (!a->toBool())
          {
            int offset = GET_B(ins);
            frame.ip += offset;
          }
          break;
        }

        case OP_JUMP_IF_TRUE:
        {
          gc<Object> a = load(frame, GET_A(ins));
          if (a->toBool())
          {
            int offset = GET_B(ins);
            frame.ip += offset;
          }
          break;
        }

        case OP_CALL:
        {
          gc<Multimethod> multimethod = vm_.getMultimethod(GET_A(ins));
          gc<FunctionObject> function = multimethod->getFunction(vm_);

          int firstArg = GET_B(ins);
          int stackStart = frame.stackStart + firstArg;
          call(function, stackStart);
          break;
        }

        case OP_NATIVE:
        {
          Native native = vm_.getNative(GET_A(ins));
          ArrayView<gc<Object> > args(stack_, frame.stackStart);
          NativeResult result = NATIVE_RESULT_RETURN;
          gc<Object> value = native(vm_, *this, args, result);

          switch (result)
          {
            case NATIVE_RESULT_RETURN:
              store(frame, GET_C(ins), value);
              break;

            case NATIVE_RESULT_THROW:
              // TODO(bob): Implement this so natives can throw.
              ASSERT(false, "Not impl.");
              break;

            case NATIVE_RESULT_CALL:
            {
              gc<FunctionObject> function = asFunction(args[0]);

              // Call the function, passing in this method's arguments except
              // the first one, which is the function itself.
              call(function, frame.stackStart + 1);
              break;
            }

            case NATIVE_RESULT_SUSPEND:
              return FIBER_SUSPEND;
          }
          break;
        }

        case OP_RETURN:
        {
          gc<Object> value = load(frame, GET_A(ins));
          callFrames_.removeAt(-1);

          // Discard any try blocks enclosed in the current chunk.
          while (!nearestCatch_.isNull() &&
                 (nearestCatch_->callFrame() >= callFrames_.count()))
          {
            nearestCatch_ = nearestCatch_->parent();
          }

          if (callFrames_.count() > 0)
          {
            // Give the result back and resume the calling chunk.
            storeReturn(value);
          }
          else
          {
            // The last chunk has returned, so end the fiber.
            result = value;
            return FIBER_DONE;
          }
          break;
        }

        case OP_THROW:
        {
          gc<Object> error = load(frame, GET_A(ins));
          if (!throwError(error)) return FIBER_UNCAUGHT_ERROR;
          break;
        }

        case OP_ENTER_TRY:
        {
          int offset = frame.ip + GET_A(ins);
          nearestCatch_ = new CatchFrame(nearestCatch_, callFrames_.count() - 1,
                                          offset);
          break;
        }

        case OP_EXIT_TRY:
        {
          nearestCatch_ = nearestCatch_->parent();
          break;
        }

        case OP_TEST_MATCH:
        {
          gc<Object> pass = load(frame, GET_A(ins));
          if (!pass->toBool())
          {
            gc<Object> error = DynamicObject::create(
                vm_.getClass(CLASS_NO_MATCH_ERROR));
            if (!throwError(error)) return FIBER_UNCAUGHT_ERROR;
          }
          break;
        }
      }
    }

    ASSERT(false, "Should not get here.");
    return FIBER_DONE;
  }

  void Fiber::storeReturn(gc<Object> value)
  {
    CallFrame& frame = callFrames_[-1];
    instruction instruction = frame.function->chunk()->code()[frame.ip - 1];
    ASSERT((GET_OP(instruction) == OP_CALL ||
            GET_OP(instruction) == OP_NATIVE),
           "Should be returning to a call or native.");

    store(frame, GET_C(instruction), value);
  }

  void Fiber::resume(gc<Object> returnValue)
  {
    // Translate VM NULL to Magpie nothing so that the caller doesn't have to
    // bother looking up the VM to get it.
    if (returnValue.isNull()) returnValue = vm_.nothing();

    storeReturn(returnValue);
    scheduler_.run(this);
  }

  void Fiber::ready()
  {
    scheduler_.add(this);
  }

  void Fiber::waitToSend(gc<Object> value)
  {
    ASSERT(sendingValue_.isNull(), "Already waiting to send a value.");
    sendingValue_ = value;
  }

  gc<Object> Fiber::sendValue()
  {
    ASSERT(!sendingValue_.isNull(), "Not sending a value.");
    gc<Object> value = sendingValue_;
    sendingValue_ = NULL;

    ready();

    return value;
  }

  void Fiber::sleep(int ms)
  {
    scheduler_.sleep(this, ms);
  }

  void Fiber::reach()
  {
    // Walk the stack.
    int numSlots = numActiveSlots();

    // Only reach slots that are still in use. We don't shrink the stack, so it
    // may have dead slots at the end that are safe to collect.
    int i;
    for (i = 0; i < numSlots; i++)
    {
      stack_[i].reach();
    }

    // For the remaining slots, clear them out now. When a new call is pushed
    // onto the stack, we allocate slots for it, but we don't clear them out.
    // This means that when a collection occurs, there may be a few slots on
    // the end of the stack that are stale: they are set to whatever they were
    // on some previous call. Since a collection may have occurred between now
    // and then, and dead slots aren't reached (see above), we may have bad
    // pointers. This clears those out so we don't get into that situation. We
    // do it here instead of in call() because call() needs to be as fast as
    // possible.
    for (; i < stack_.count(); i++)
    {
      stack_[i] = gc<Object>();
    }

    for (int i = 0; i < callFrames_.count(); i++)
    {
      callFrames_[i].function->reach();
    }
  }

  void Fiber::trace(std::ostream& out) const
  {
    out << "[fiber " << id_ << "]";
  }

  void Fiber::call(gc<FunctionObject> function, int stackStart)
  {
    // Allocate slots for the method.
    stack_.grow(stackStart + function->chunk()->numSlots());
    callFrames_.add(CallFrame(function, stackStart));
  }

  bool Fiber::throwError(gc<Object> error)
  {
    // If there is nothing to catch it, end the fiber.
    if (nearestCatch_.isNull())
    {
      // TODO(bob): This should be consolidated and moved somewhere else.
      // Ideally, the stack trace would be shown after the error.
      // Print a stack trace.
      for (int i = callFrames_.count() - 1; i >= 0; i--)
      {
        CallFrame& frame = callFrames_[i];
        // -1 because ip has already advanced to the next instruction.
        int line = -1;
        gc<SourceFile> source = frame.function->chunk()->locateInstruction(
            frame.ip - 1, line);

        if (!source.isNull() && line != -1)
        {
          // TODO(bob): Can do better here. Would be nice to show enclosing
          // method/function/async block.
          std::cerr << "[" << source->path() << " line " << line
                    << "]" << std::endl;
          std::cerr << source->getLine(line) << std::endl;
        }
      }

      vm_.printUncaughtError(this, error);
      callFrames_.clear();

      return false;
    }

    // Unwind any nested callframes above the one containing the catch clause.
    callFrames_.truncate(nearestCatch_->callFrame() + 1);

    // Jump to the catch handler.
    CallFrame& frame = callFrames_[-1];
    frame.ip = nearestCatch_->offset();

    // The next instruction is a pseudo-op identifying where the error is.
    instruction errorIns = frame.function->chunk()->code()[frame.ip];
    ASSERT(GET_OP(errorIns) == OP_MOVE,
        "Expect pseudo-instruction at beginning of catch code.");
    int errorSlot = GET_A(errorIns);
    store(frame, errorSlot, error);
    frame.ip++;

    // Discard the try block now that we are outside of it.
    nearestCatch_ = nearestCatch_->parent();

    return true;
  }

  gc<FunctionObject> Fiber::loadFunction(CallFrame& frame, int chunkSlot)
  {
    Chunk& chunk = *frame.function->chunk();
    gc<Chunk> functionChunk = chunk.getChunk(chunkSlot);
    gc<FunctionObject> function = FunctionObject::create(functionChunk);

    // Capture any needed upvars from the enclosing procedure.
    // TODO(bob): If most of the upvars are not captured, we could do something
    // more efficient here than a series of essentially no-op
    // pseudo-instructions.
    for (int i = 0; i < functionChunk->numUpvars(); i++)
    {
      instruction ins = chunk.code()[frame.ip++];
      ASSERT(GET_OP(ins) == OP_GET_UPVAR, "Bad closure pseudo-instruction.");

      if (GET_C(ins) == 1)
      {
        gc<Upvar> upvar = frame.function->getUpvar(GET_A(ins));
        function->setUpvar(i, upvar);
      }
    }

    return function;
  }

  int Fiber::numActiveSlots() const
  {
    if (callFrames_.count() == 0) return 0;

    const CallFrame& frame = callFrames_[-1];
    return frame.stackStart + frame.function->chunk()->numSlots();
  }

  void CatchFrame::reach()
  {
    parent_.reach();
  }
}
