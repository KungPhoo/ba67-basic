0123456789012345678901234567890123456789012345678901234567890
# BA67 - 80s BASIC Interpreter
## About
BA67 (pronounced BASIC SEVEN) is a standlone BASIC interpreter,
that can be used as a daily tool on a personal computer.

It is inspired by the Commodore BASIC V7 ©1977 from the C128.
Three times the digit 7 led to it's name BA67.

If you're familiar with the C64/C128, you will quickly find
yourself comfortable with this interpreter. But BASIC is also
a very easy to learn programming language as well as an
operating system. So it suits perfectly for beginners as well.

Many have programmes BASIC interpreters before, so why another?
Well, they all had some sort of limitations, that I don't want.

This BASIC is compatible with COMMODORE's BASIC V7,
but also features some improvements.

Here are the key features:
- Compatible with the famous Commodore's BASIC
- Variable names can be longer than 2 characters
- Unicode strings (no PETSCII or other exotic charsets)
- 64 bit precission for numbers
- 64 bit integers
- Integers are stored as such. Conversion is performed internally
- Direct access to your local filesystem
- Modular programming (See keyword `MODULE`)

-------------------------------------------------------------
## Goal
Back in the 80s, everyone could start a computer and program
a simple
```
10 PRINT "HELLO WORLD"
20 GOTO 10
```

These days there's so many languages and programs to install
before the fun starts, that many do not even bother to
try it.

The goal of this project is to get people to start
programming.

It aims to provide a BASIC language, that sticks to a very
widespread standard, keep backwards compatiblity and focus
on an easy to learn language with easy to understand syntax
and few keywords.

Keeping the backwards compatiblity also enables you to have
AI generators answer your questions.

Complex operators, module definitions, structures and
inheritance are excluded on purpose. Yet, the language
should offer a modular approach for complex programs
without introducing complex syntax.

The very final goal would be to have a standalone hardware,
that boots to this BASIC.

Graphics and sound should be added.

For the graphics an 40x25 character graphics with 16 colors
is set. Each character is monochrome. Characters can be
easily generated with the `CHARDEF` command and supports
the full unicode range (uses a map internally).

For the sound, the ABC music notation is used. For sound
effects, DrPetter's SFXR is used. See `PLAY` and `SOUND`.

-------------------------------------------------------------
## Syntax
This interpreter tries to imitate and extend
COMMODORE's BASIC V7.

The parser is case sensitive, but the line input uppercases
everything outside of quotes.

Keywords must be separated with spaces, operators or braces.
The COMMODORE allowed `LETTER=1`, which was interpreted as
`LET TER=1` instead of `LET LETTER=1`. Also `PRINT LETTER`
was not possible, because the LET part was interpreted as a
command instead of the varaible name.

In order to avoid confusions, this BASIC requires separating
commands from variables.

-------------------------------------------------------------
## Editor
The editor imitates the C64/C128 text editor. Use the ESC
key to abort a program execution or the listing of `CATALOG`
or `LIST`.

The F1 .. F12 keys are preprogrammed with some BASIC keywords
and can be configured using the KEY command.

Use Alt+INS key to toggle between insert mode and overwrite
mode. Use the INS key alone to insert a single character
space.

The ScrollLock key can be used to pause the program. That is
especially usefull when printing the `LIST` command.

-------------------------------------------------------------
## Numbers
Number constants can be given in scientific notation
(-1.3e-5) or as floating point input (1.23). The decimal
separator is the period symbol, regardless of the computer's
locale settings.

Intergers can be given up to 64 bits. A prefixed `$` is
interpreted as a hex number. `a = $7ffffff`.

-------------------------------------------------------------
## Unicode
This interpreter has full Unicode support. Even emoji 😀.
The Windows console, however, cannot display characters
above 0xffff and will display two characters instead ☹.

LEN, MID$ etc. work with unicode codepoints, so
LEN("äöü") = 3, even though the parser stores 6 utf-8
bytes, internally.


-------------------------------------------------------------
## Variables
Variables can be double, integer or string variables and 
arrays of these.
Integers have a % postfix, where strings end with $.
The double variable 'a' and the integer variable 'a%' are
two separate variables. Also, arrays can have the same name
as non-array variables and are treated as separate variables.
(That's compatible with CBM BASIC)

There are built-in variables, that will be updated before
each statement is evaluated. These are:

|Variable | Value |
|-----------------|
|TI       |current system time in 1/60 sec. |

## Files
The class that derives from Os should set the start
directory to a location, where BASIC programs are to
be stored.

When starting, the interpreter will load and run the
file "boot.bas" from the start directory and call `NEW`
to clear all variables.

You can edit and save this file for your needs and e.g.
adjust the `KEY` shortcuts etc.

The interpreter can load files without line numbers and will
numerate them automatically.

-------------------------------------------------------------
## Keywords, Commands, Functions
The following is a list of the BASIC hardcoded language
tokens you can use to write programs. They are split in 3
groups, where the first two can be considered as one - only
if you want to look at the source code, you will find these
two different groups.

Functions always take at least one argument in braces.

-------------------------------------------------------------
## Keywords
### DATA
**Usage:**
`DATA value, value, string, "string with spaces", ...`

Provides data variables to be used with the READ keyword.

### DEF FN
**Usage** `DEF FN NAME(ARG[, ARG2, ...]) = ARG+ARG2...*`

Provides a way to define user functions, that can be called.
Internally, the function name always starts with FN, so this
is the only place, where `FN FOO` and `FNFOO` are valid and
equal.

### DELETE
**Usage:** `DELETE from [- to]`

Deletes the line numbers in the given range.


### DIM
**Usage:** `DIM var(size)`

Allocates an array with the specified size.

Example:
```basic
DIM A(10)
```

### ON
**Usage:** `ON expr GOTO line1, line2, ...`
            `ON expr GOSUB line1, line2, ...`

Transfers execution to one of the specified lines based on
the value of `expr`.

Example:
```basic
ON X GOTO 100, 200, 300
```

### FN
See the `DEF FN` description, please.

### FOR
**Usage:** `FOR var = start TO end [STEP increment]`

Begins a loop with an optional increment.

Example:
```basic
FOR I = 1 TO 10 STEP 2
```

### GOSUB
**Usage:** `GOSUB line`

Calls a subroutine at the specified line number. Execution
resumes after a `RETURN`.

Example:
```basic
GOSUB 200
```

### GOTO
**Usage:** `GOTO line`

Jumps to the specified line number unconditionally.

Example:
```basic
GOTO 100
```

### IF
**Usage:** `IF condition THEN statement`

Executes `statement` if `condition` evaluates as true. If
the condition is false, the execution of this line stops.
This differs from other BASICs.

Example:
```basic
IF X = 10 THEN PRINT "Hello"
```

### LET
**Usage:** `LET var = expr`

Assigns a value to a variable. `LET` is optional.

Example:
```basic
LET X = 5
```

### KEY
**Usage:** `KEY [index, string]`
The KEY keyword lets you specify the text, that's printed,
when you press any of the F1..F12 function keys. Without
arguments, the keyword lists the current keyboard shortcuts.

### NEXT
**Usage:** `NEXT var`

Marks the end of a `FOR` loop, increasing `var` by `STEP`.

Example:
```basic
NEXT I
```

### READ
**Useage:** `READ var[, var [, var2] ]`

Reads a variable from the next DATA keyword.

### RESTORE
**Usage:** `RESTORE [linenumber]`

Restores the next READ's DATA to the given line number or,
if omitted, the start of the program.

### RETURN
**Usage:** `RETURN`

Returns from a subroutine invoked with `GOSUB`.

Example:
```basic
RETURN
```

### STEP
**Usage:** Used in `FOR` loops to define an increment.

Example:
```basic
FOR I = 1 TO 10 STEP 2
```

### THEN
**Usage:** Used in conjunction with `IF` to specify the
action when the condition is true.

Example:
```basic
IF A > B THEN GOTO 100
```


### TO
**Usage:** Used with `FOR` to specify the loop end value.

Example:
```basic
FOR I = 1 TO 5
```



-------------------------------------------------------------

## Commands
### ABOUT
Prints the 'about this project' and license message.

### AUTO
Enables automatic line numbering. This aids you when writing
programs by printing the next line number at the input prompt
after a line has been programmed by adding `n` to the
previously entered line number.

Calling AUTO with no argument disables this feature.

When a line already exists, BA67 will print the contents of
this line to prevent you from overwriting existing code.

**Usage:** `AUTO [n]`_

### CLR
Clears all variables.

**Usage:** `CLR`

### COLOR
Set text or background color. The screen is an indicator
what colour to change.
```
0 or 6: Background (C128: 0)
1 or 5: Text color (C128: 5)
4     : Border color (only if available)
```
The default color values are:
```
1  Black
2  White
3  Red
4  Cyan
5  Purple
6  Green
7  Blue
8  Yellow
9  Orange
10  Brown
11 Light Red
12 Dark Gray
13 Medium Gray
14 Light Green
15 Light Blue
16 Light Gray
```

**Usage:** `COLOR screen, color`


### CHDIR
Change into the given directroy.

**Usage:** `CHDIR "subdir"`
The CHDIR command also supports wildcard characters.
use CHDIR ".." to go one directory level up.

### CATALOG
Lists all files and directories of the current directory.

### CHAR
Positions the cursor position to `column,line`, where the top
left screen corner is at position `0,0`.
Then prints the given string at that position and leaves the
cursor at the end of the printed string's position.
Other BASICs have a command `LOCATE` or `PRINT AT` for this.

The parameter colour_source is just for compatibility and
will be ignored.

**Usage:**
`CHAR color_source, column, line, text$[, invers]`

### CHARDEF
**Monochrome**

Defines the pixels of a character symbol.
You can define any unicode character with this command.
Each parameter is a byte (8 bits), that describe the
horizontal pixels of a line. You must pass 8 lines.

Alternatively, you can pass all 8 bytes as a single number,
since BA67 supports 64 bit integers.
So `CHARDEF "A", $ff22334455667788` is valid.

Use the `CHR$()` command, if you want to give the unicode
code point as a value.

**Multicolor**

If your pixel size ix 8x8 (default) and you pass 8 values
of 32 bits. They are interpreted as 16 bit color indices.
So each byte represents 2 pixels. In order to distinguish
these 8 values from the 8 bytes of a monochrome character,
one value must be greater than $ff.

Example:
```
10 CHARDEF "$",24,36,32,112,32,98,92,0
20 READ C$, A,B,C,D,E,F,G,H
30 CHARDEF C$, A,B,C,D,E,F,G,H
40 PRINT "$$$ ###"
100 REM MULTICOLOR IMAGE
110 DATA "#"
120 DATA $11111144
130 DATA $11144444
140 DATA $11144444
150 DATA $14444444
160 DATA $14444446
170 DATA $14446666
180 DATA $44446666
190 DATA $44466666
```


`CHARDEF "#", $00, $11, $11, $00,  $11, $22, $22, $11,  $12, $33, $33, $21,  $13, $44, $44, $31,  $14, $55, $55, $41,  $15, $66, $66, $51,  $01, $77, $77, $10,  $00, $18, $81, $00`


#### For coders:
If you reimplement BA67 in your own project and your
character height is defined to 16, but you only pass
8 lines, each line will be duplicated.
See ScreenInfo structure in the code.

**Useage:**
```
10 CHARDEF "$",24,36,32,112,32,98,92,0
20 PRINT "$$$$"
RUN
```

### END
Terminates program execution.

**Usage:** `END`

### EXIT
Exits the interpreter.

**Usage:** `EXIT`

### FAST
Enables fast mode for this `MODULE`. That's the default
speed. No delay will be added. See also `SLOW`.

**Usage:** `FAST`

### FIND
Searches the current module's program listing for the given
search string. The search is performed case-insensitive
and you can use the wildcards '*' and '?'.
A '*' is appended at the front and the back, internally.

The command will act as `LIST`, but only list the lines where
the search string was matches.

**Usage:** `FIND "print*hello world"`

### GET
Gets a key press from the keyboard buffer. Will return
an empty string, if the buffer is empty.

**Usage:** `GET a$ [, b$, ...]`


### INPUT
Prompts for user input.

**Usage:** `INPUT var`

Example:
```basic
INPUT X
```

### LIST
Displays the program listing.

**Usage:** `LIST`

### LOAD
Loads a program from the disk drive. You can use the forward
slash character to specify directories like `dir/file.bas`.
Wildcard characters '*' and '?' are supported in folders as
well as in file names.

**Usage:** `LOAD "bas*folder/*.bas"`

### MOVSPR
Moves a sprite to a given screen location.
`MOVSPR nr, x, y`

### NEW
Clears the current program from memory.

**Usage:** `NEW`

### MODULE
Modules are a great way to modulize complex code and reuse
existing listings. The interpreter holds a list of variable
spaces for each module as well as a program counter
(listing execution position) for each module.

With the MODULE keyword you can switch into the variable
space of another module. The program counter, however, still
is in the old module! This way you can modify code in the
sub-module. Calling RUN execute the module's code until it
calls END or reaches the end of it's listing.

After RUN or END, the current module is switched back to the
calling module.

It's a common technique to switch to a module, `LOAD` the
code, then END back to the main program. When required,
the module code can be executed using `MODULE name: RUN`.

Passing arguments back and forth can be done by prefixing
the variable name with the module name followed by a period
operator. `module.varname = 123`, which can be called from
any other module.

**Usage:** `MODULE name`

Example:
Type this example line by line into the interpreter:
```
10 M = 1
20 PRINT "IN MAIN "; M  : REM M=1
30 MODULE MYMOD
40 M = 2                : REM MYMOD.M=2
50 RUN                  : RUNS MYMOD'S CODE
60 PRINT "IN MAIN "; M  : REM M=1

REM THE MODULE KEYWORD WILL
REM BRING US INTO THE MODULE'S SPACE
MODULE MYMOD


10 PRINT "IN MODL "; M    : REM (MYMOD.) M=2
30 M=3                    : REM (MYMOD.) M=3
40 PRINT "IN MODL "; M    : REM (MYMOD.) M=3
50 END                    : REM BACK TO MAIN MODULE

REM THE END KEYWORD WILL
REM BRING US BACK TO THE MAIN MODULE
END

REM NOW RUN THE MAIN PROGRAM
RUN
```

Output:
```
IN MAIN 1
IN MODL 2
IN MODL 3
IN MAIN 1
```




### PLAY
The PLAY command plays a music score in the background of
your program. The music string is in ABC music notation.

#### ABC Music Notation - Basics
ABC notation is a simple text-based format for writing
music.

#### Basic Structure
An ABC tune consists of metadata headers followed by the
music notation.

#### Example:
```
X:1
T:Sample Tune
M:4/4
L:1/8
K:C
C D E F | G A B c |
```

#### Key Components:
- `X:` - Tune index (required).
- `T:` - Title of the tune.
- `M:` - Time signature (e.g., `4/4`, `6/8`).
- `L:` - Default note length (e.g., `1/8` for eighth notes).
- `K:` - Key signature (e.g., `C`, `Gm`).

BA67 will always prefix this default header for you:
```
X:1
T:Song
M:4/4
L:1/4
K:C
```
You can simply overwrite the parts, that need to be changed.
Also, BA67 replaces any semicolon ';' with a newline
character, so you don't have to break short lines with
`CHR$(13)` - the newline character.

#### Notes & Notation
- Letters `A-G` represent notes.
- Lowercase letters (`a-g`) represent higher octaves.
- `|` represents a bar line.
- Numbers (`2`, `3`, etc.) extend note duration
  (e.g., `C2` is a half note in `4/4`).
- `z` represents a rest.

#### Chords & Decorations
- Chords: `[CEG]` plays a C major chord.
- Grace notes: `{d}` adds a grace note before the next note.
- Ties: `C-C` connects two notes.

#### Instruments
- `I:instrument=X` where X is the instrument number of the
  MIDI Program number. The codes are listed on
  [midiprog.com](https://midiprog.com/program-numbers/) or
  [Wikipedia](https://en.wikipedia.org/wiki/General_MIDI).
  
**Example**
```
X: 1
T: Smokey Waters
M: 4/4
L: 1/4
K: Em
%%MIDI program 29
E, G, A,2 | E, G, B,1/2-A,3/4 | -A,4
```

#### Multi-Voice
ABC notation allows multiple voices to be written together
using `V:`.

```
PLAY "V:1; C D E F | G A G F |; V:2; G, A, B, C | D E D C |"
```
#### Editors
You might want to have an editor to play around. Why not
write one yourself 🎵.
There are many editors for ABC music notation. Some
are even hosted online like on
[drawthedots.com](https://editor.drawthedots.com/).


#### More Features
The percent sign `%` is used to indicate a comment.

ABC notation supports lyrics, ornaments, and multi-voice
arrangements, making it a flexible format for digital
sheet music.

For more details, visit
[Wikipedia](https://en.wikipedia.org/wiki/ABC_notation)
or [abcnotation.com](https://abcnotation.com) (which,
I'm afraid, is quite loaded with commercials).
Here's the standard: [abcnotation.com/wiki](https://abcnotation.com/wiki/abc:standard:v2.1)

#### Technical Background
BA67 uses two command line tools for the `PLAY` command. Both must
be installed and in the PATH variable. On Windows, the programs must
be relative to the executable "bin\BA67.exe":
"..\3rd-party\abcMIDI\bin\abc2midi.exe"
"..\3rd-party\fluidsynth\bin\fluidsynth.exe"

ABC2MIDI is a GPL software and will thuss not be distributed.
[abcmidi](https://ifdo.ca/~seymour/runabc/top.html) or
[github](https://github.com/sshlien/abcmidi)

For Fluidsynth, also a soundfont file must be present.
Currently BA67 searches for:

On Windows:
"..\3rd-party\fluid-soundfont-3.1\FluidR3_GM.sf2"

otherwise
"FluidR3_GM.sf2"
TODO: Find a better place where to install that file.

If any of these files cannot be found, BA67 uses a very, very
primitive ABC music notation parser, that generates the melody
using a sine wave. So, if `PLAY "C D E"` sounds terribly bad,
you're probably missing any of the above files.


### POKE
Puts a byte to a virtual memory address. This is only for
compatibility. The `PEEK` command can retrieve the value.
Ihe memory has no influence on the machine, screen or other
devices.

**Usage:** `POKE addr, byte`

Example:
```basic
POKE 1234, 128
```

### PRINT
Outputs text or values to the screen.

**Usage:** `PRINT expr`

Example:
```basic
PRINT "Hello, World!"
```

### PRINT USING
Outputs text or values as a formated string to the screen.

**Usage:** `PRINT USING format; expr [, expr ...]`

Example:
```basic
10 PRINT USING "+##";1                 : REM "+ 1"
20 PRINT USING "#.##+";-0.01           : REM "0.01-"
30 PRINT USING "-.##";-0.1             : REM "-0.10"
40 PRINT USING "##.#-";1               : REM " 1.0"
50 PRINT USING "####";-100.5           : REM "-100"
60 PRINT USING "####";-1000            : REM "****"
70 PRINT USING "#.##";-4E-03           : REM "****"
80 PRINT USING "##.";10                : REM " 10"
90 PRINT USING "###.##"; 100           : REM "100.00"
100 PRINT USING "##.##";10.4           : REM "10.40"
110 PRINT USING "#,###.##";10000.0009  : REM "********"
120 PRINT USING "##,##";-1             : REM "  -1"
130 PRINT USING "##,##";-10            : REM " -10"
140 PRINT USING "##,##";-100.9         : REM "-101"
150 PRINT USING "+#.#^^^^";1           : REM "+1.0e+00"
160 PRINT USING "#^^^^";1.5E+11        : REM "2e+11"
1010 PRINT USING "##.##";"CBM"         : REM "CBM"
1020 PRINT USING "X##=##X";"CBM"       : REM "X CBM X"
```

### REM
Adds a comment in the program.

**Usage:** `REM comment`

Example:
```basic
REM This is a comment
```

### RENUMBER
Renumbers the program lines.

**Usage:** `RENUMBER [start, increment]`
(Optional arguments: `start`, `increment`)

### RUN
Executes the program. Optionally a line number can be passed
as the start of the processing.

If the program switched to a `MODULE` earlier, this will set
the active program listing to the module's code.

Optionally, a line number can be specified.

**Usage:** `RUN [start_line_number]`

### SAVE
Saves the BASIC program of the current module to a file on
the disk drive. The file extension ".BA67" is recommended.
If no file extension is given, it will be appended.

**Usage:** `SAVE "test.bas"`

### SCNCLR
Clears the screen and puts the cursor in the top left corner.
The optional parameter is ignored.
**Usage:** `SCNCLR [n]`

### SLOW
Slow mode is enabled. This adds a delay for every
instruction to aproximately simulate the speed of a C64.

### SOUND
Plays a sound in the background. The parameters are passed
as a string, that's separated by spaces, commas or colons.

The voice is one of the 64 voice channels to mix the sounds.

BA67 uses DrPetter's SFXR. More details on the parameters
can be found on his homepage:
https://www.drpetter.se/project_sfxr.html

Optionally different sound engines might be used during
compilation. See Build chapter.

Here's the list of parameters:
```
*Wave Shape*
wave_type      : 0 = square, 1 = sawtooth, 2 = sine, 3 = noise

*Envelope*
env_attack   : [0 +1] Attack time
env_sustain  : [0 +1] Sustain time
env_decay    : [0 +1] Decay time
env_punch    : [0 +1] Sustain punch

*Tone*
base_freq    : [0 +1] Start frequency
freq_limit   : [0 +1] Min frequency cutoff
freq_ramp    : [-1+1] Slide (SIGNED)
freq_dramp   : [-1+1] Delta slide (SIGNED)

*Square wave duty* (proportion of time signal is high vs. low)
duty         : [0 +1] Square duty
duty_ramp    : [-1+1] Duty sweep (SIGNED)

*Vibrato*
vib_strength : [0 +1] Vibrato depth
vib_speed    : [0 +1] Vibrato speed
vib_delay    : [0 +1] Vibrato delay

*Filter*
filter_on    : [1/0] don't know if this has any influence.

*Low-Pass Filter*
lpf_resonance: [0 +1] Low-pass filter resonance
lpf_freq     : [0 +1] Low-pass filter cutoff
lpf_ramp     : [0 +1] Low-pass filter cutoff sweep (SIGNED)

*High-Pass Filter*
hpf_freq     : [0 +1] High-pass filter cutoff
hpf_ramp     : [0 +1] High-pass filter cutoff sweep (SIGNED)

*Flanger*
pha_offset   : [-1+1] Flanger offset (SIGNED)
pha_ramp     : [-1+1] Flanger sweep (SIGNED)

*Repeat*
repeat_speed : [0 +1] Repeat speed

*Tonal change*
arp_speed    : [0 +1] Change speed
arp_mod      : [-1+1] Change amount (SIGNED)

*Volume*
volume       : [0 +1] Sound volume
```
**Usage:** SOUND voice, play$
```
REM play noise wave with default values
SOUND 1, "wave_type:3"
```

### SPRDEF
Defines a sprite. You have 256 sprites to use. They are
overlaid over the character graphics and can be moved on
a per pixel basis.

In order to define a sprite, first you define characters.
Then you assign the character images to a sprite number.
A sprite consists of 3x2 character images.
You can select any color index to be transparent with the
3rd parameter. If you ommit it, the sprite will not be
transparent.

`SPRDEF nr, chars$`


### SPRITE
Changes the visibility and characteristics of a sprite.
The `on` parameter enables 1 or disables 0 the sprite.

If the bitmap is monochrome, the `color` sets the color
of the sprite. If it's a multicolor sprite, this sets
the transparent color index 1..16.

The `prio` parameter is currently not used.

The flags `x2` and `y2` enable 1 or disable 0 the scaling
of the sprite to twice it's size.
`SPRITE nr, on, color, prio, x2, y2`

Sprites can be moved with `MOVSPR`.

### STOP
Stops the program, is if the escape key was pressed with the
`?BREAK IN linenumber` message.

### SYS
If a string is given, the system's command is executed.
The string must be quoted. Otherwise we could not use
variables in the command string.
If a number is given, the command prints an error message.

**Usage:**
`SYS "wget " + CHR$(22) + "www.microsoft.com" + CHR$(22)`




-------------------------------------------------------------
## Functions
### ABS
Returns the absolute value of a number.

**Usage:** `ABS(expr)`

### ASC
Returns the ASCII code of a character.

**Usage:** `ASC("char")`

### ATN
Returns the arctangent of a number.

**Usage:** `ATN(expr)`

### CHR$
Returns the character corresponding to an ASCII code.

**Usage:** `CHR$(code)`

### COS
Returns the cosine of an angle in radians.

**Usage:** `COS(expr)`

### DEC
Converts a hex string to an integer number.

**Usage:** `PRINT DEC("FF")`

### EXP
Returns `e` raised to a power.

**Usage:** `EXP(expr)`

### FRE
Returns the amount of memory for PEEK and POKE operations.
On a C64, this would print the memory available for your
BASIC program.

**Usage:** `FRE(0)`

### HEX$
Returns a string representation of an integer number.

**Usage:** `PRINT HEX$( $ff001234 )`

### INSTR
**Usage: `pos=INSTR(haystack$, needle$, [startpos])`
Returns the first occourence of `needle$` in `haystack$``.
The first character is the index 1. A value 0f 0 indicates,
that no match was found.

### INT
Returns the integer portion of a number.

**Usage:** `INT(expr)`

### LCASE$
Converts the string to lower case.

**Usage:** `LCASE$(s$)`

### LEFT$
Returns the leftmost `n` characters of a string. A character
is a unicode code point.

**Usage:** `LEFT$(str, n)`

### LEN
Returns the length of a string by counting the unicode code
points.

**Usage:** `LEN(str)`

### LOG
Returns the natural logarithm of a number.

**Usage:** `LOG(expr)`

### MID$
Extracts a substring from a string. Each character is a
unicode code point.

**Usage:** `MID$(str, start, length)`

### PEEK
Returns the value from a memory address.

**Usage:** `PEEK(addr)`

### POS
Returns the current horizontal cursor position.
The first column returns the number 0.
An argument must be passed, is however not evaluated.

**Usage:** `POS(0)`

### POSY
Returns the current vertical cursor position.
The first row returns the number 0.
An argument must be passed, is however not evaluated.

**Usage:** `POSY(0)`

### RIGHT$
Returns the rightmost `n` characters of a string. A character
is a unicode code point.

**Usage:** `RIGHT$(str, n)`

### RND
Returns a random number.

**Usage:** `RND(expr)`

### SGN
Returns the sign of a number (-1, 0, or 1).

**Usage:** `SGN(expr)`

### SIN
Returns the sine of an angle in radians.

**Usage:** `SIN(expr)`

### SPC
Outputs spaces in a `PRINT` statement.

**Usage:** `SPC(n)`

### SQR
Returns the square root of a number.

**Usage:** `SQR(expr)`

### STR$
Converts a number to a string.

**Usage:** `STR$(expr)`

### TAB
Moves the cursor to a specified column in `PRINT`.

**Usage:** `TAB(n)`

### TAN
Returns the tangent of an angle in radians.

**Usage:** `TAN(expr)`

### UCASE$
Converts the string to upper case.

**Usage:** `UCASE$(s$)`

### VAL
Converts a string to a number.

**Usage:** `VAL(str)`

### XOR
Binary XOR operator.

**Usage:** `A = XOR(3,2)`


## Reserved Commands
Here's a list of commands and constants, that might be
implemented once. So don't use these as veriable names.
But then, also don't wait for their implementation ;)
- EL, ER, ERR$, DS, DS$
- SCRATCH
- LOF
- DO, LOOP, UNTIL, WHILE
- TRAP
- REDIM
- BEGIN, BEND
- ROUND
- SPLIT
- PEN, JOY, POT
- STRING$()
- CONCAT
- CATALOG$
- SPRITE, RSPRITE, RSPRPOS, RSPRCOL, MOVSPR
- SPRDEF
- WINDOW, RWINDOW
- OPEN AS, GET#, INPUT#, PRINT#, CLOSE

-------------------------------------------------------------
This manual serves as a reference for all available
commands, keywords, and functions in BA67.
For additional details or examples, consult the official
documentation.


