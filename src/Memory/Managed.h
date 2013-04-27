#pragma once

#include <iostream>

#include "Common.h"
#include "Memory/Memory.h"

namespace magpie
{
  class String;

  // Base class for any object that is memory managed by the garbage collector
  class Managed
  {
  public:
    Managed() {}

    virtual ~Managed() {}

    gc<String> toString() const;

    // Gets the forwarding pointer that this object has been replaced with if
    // it has been. Otherwise returns NULL.
    virtual Managed* getForwardingAddress() const { return NULL; }

    // This will be called by the garbage collector when this object has been
    // reached. Subclasses should override this and call Memory::copy() on any
    // gc<T> references that the object contains.
    virtual void reach() {}

    virtual void trace(std::ostream& out) const;

    void* operator new(size_t s);

  private:

    NO_COPY(Managed);
  };

  inline std::ostream& operator <<(std::ostream& out, const Managed& object)
  {
    object.trace(out);
    return out;
  };
}
