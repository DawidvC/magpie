package com.stuffwithstuff.magpie.interpreter;

import com.stuffwithstuff.magpie.parser.Position;

public interface InterpreterHost {
  void print(String text);
  void runtimeError(Position position, String message);
}
