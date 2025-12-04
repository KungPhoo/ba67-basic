#include "about.h"

//                                    40                                      80
// 45678901234567890123456789012345678901234567890123456789012345678901234567890
const char* about::text() {
    static const char* buffer = R"(
       BA67 - BASIC Interpreter
       =========================
       Version {VERSION}
       Build   {BUILD}
       Copyright (c) 2025
       Dream Design Entertainment
.
BA67 (pronounced  "BASIC  SEVEN")  is  a
modern  BASIC  interpreter  designed for
full   backward    compatibility    with
      ** Commodore BASIC V7.0 **
while  bringing it to   modern  hardware
and technology.
For  more  details,   see **README.md**.
.
.
           ** License: **
       -------------------------
You  may  use,  modify,  and  distribute
any     part    of     this      project
        ** free of charge **,
provided   that  this   ABOUT    message
remains   included   and  is   displayed
when   running   the  "ABOUT"   command.
.
.
         ** Disclaimer: **
       -------------------------
.
This  software  is  provided  **as is**,
without  any  warranty  of any kind. The
authors  are  **not  responsible**   for
any  damage,  data loss  or other issues
arising  from  its  use. By  using  this
software,    you    agree    that    you
**cannot  hold  us  liable**   for   any
issues that may occur.
.
.
           ** Challenge: **
       -------------------------
If  you like  this project,  please take
some  time to  read  these  two  verses:
.
1.JOHN 1:8-10             ROMANS 3:23-25
.
)";
    return buffer;
}
