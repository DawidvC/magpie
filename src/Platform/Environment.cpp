#include "Common.h"
#include "Platform/Environment.h"
#include "Platform/Path.h"

namespace magpie
{
  gc<String> locateModule(gc<String> programDir, gc<String> name)
  {
    // Build a relative path from the module name.
    gc<String> relative = name->replace('.', path::separator());
    relative = String::format("%s.mag", relative->cString());

    // Try to find it relative to the program first. (This will be null if
    // we're running in the REPL.)
    if (!programDir.isNull())
    {
      gc<String> path = path::join(programDir, relative);
      if (path::fileExists(path)) return path;
    }

    // Otherwise, try to find it in the system libraries.
    gc<String> path = path::join(getCoreLibDir(), relative);
    if (path::fileExists(path)) return path;

    // If we got here, it couldn't be located.
    return NULL;
  }
}
