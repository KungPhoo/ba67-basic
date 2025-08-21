// UTF-8 äöüß

// Modifier states
let shiftActive = false;
let altActive = false;
let ctrlActive = false;

// Key definition array: [label, key, code]
// key-codes: https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values
const keys = [
    //label key      code
    // ---------------------
    ['☒', '\u001b', 'Escape'],
    ['↑', 'code', 'ArrowUp'],
    ['↓', 'code', 'ArrowDown'],
    ['←', 'code', 'ArrowLeft'],
    ['→', 'code', 'ArrowRight'],
    ['F1', 'code', 'F1', 'F2'],
    ['F3', 'code', 'F3', 'F4'],
    ['F5', 'code', 'F5', 'F6'],
    ['F7', 'code', 'F7', 'F8'],
    ['⌫', '\b', 'Backspace'],

    ['1', '1', '1',],
    ['2', '2', '2',],
    ['3', '3', '3',],
    ['4', '4', '4',],
    ['5', '5', '5',],
    ['6', '6', '6',],
    ['7', '7', '7',],
    ['8', '8', '8',],
    ['9', '9', '9',],
    ['0', '0', '0',],

    ['q', 'q', 'Q',],
    ['w', 'w', 'W',],
    ['e', 'e', 'E',],
    ['r', 'r', 'R',],
    ['t', 't', 'T',],
    ['y', 'y', 'Y',],
    ['u', 'u', 'U',],
    ['i', 'i', 'I',],
    ['o', 'o', 'O',],
    ['p', 'p', 'P',],

    ['a', 'a', 'A',],
    ['s', 's', 'S',],
    ['d', 'd', 'D',],
    ['f', 'f', 'F',],
    ['g', 'g', 'G',],
    ['h', 'h', 'H',],
    ['j', 'j', 'J',],
    ['k', 'k', 'K',],
    ['l', 'l', 'l'],
    ['↲', '\r', 'Enter'],

    ['⇧', 'code', 'ShiftLeft'],
    ['z', 'z', 'Z'],
    ['x', 'x', 'X'],
    ['c', 'c', 'C'],
    ['v', 'v', 'V'],
    ['b', 'b', 'B'],
    ['n', 'n', 'N'],
    ['m', 'm', 'M'],
    [':', ':', 'Colon'],
    [';', ';', 'Semicolon'],

    ['<', '<', 'Less'],
    ['>', '>', 'Greater'],
    ['=', '=', 'Equals'],
    [' ', ' ', 'Space'],
    [' ', ' ', 'Space'],
    [' ', ' ', 'Space'],
    ['@', '@', 'At'],
    [',', ',', 'Comma'],
    ['.', '.', 'Period'],
    ['?', '?', 'Question Mark'],

    ['⎈', 'code', 'ControlLeft'],
    ['⎇', 'code', 'AltLeft'],
    ['⇤', 'code', 'Home'],
    ['⇥', 'code', 'End'],
    ['⎀', 'code', 'Insert'],
    ['+', '+', 'Plus'],
    ['-', '-', 'Minus'],
    ['∗', '*', 'Multiply'],
    ['/', '/', 'Slash'],
    ['^', '^', 'Power'],
];


const shiftedKeys = [
    //label key      code   
    // ---------------------
    ['☒', '\u0027', 'Escape'],
    ['↑', 'code', 'ArrowUp'],
    ['↓', 'code', 'ArrowDown'],
    ['←', 'code', 'ArrowLeft'],
    ['→', 'code', 'ArrowRight'],
    ['F2', 'code', 'F2'],
    ['F4', 'code', 'F4'],
    ['F6', 'code', 'F6'],
    ['F8', 'code', 'F8'],
    ['⌫', '\b', 'Backspace'],

    ['!', '!', 'Exclamation'],
    ['"', '"', 'Quotes'],
    ['#', '#', 'Hash'],
    ['$', '$', 'Dollar'],
    ['%', '%', 'Percent'],
    ['&', '&', 'Ampersand'],
    ["/", "/", 'Divide'],
    ['(', '(', '('],
    [')', ')', ')'],
    ['0', '0', '0'],

    ['Q', 'Q', 'Q'],
    ['W', 'W', 'W'],
    ['E', 'E', 'E'],
    ['R', 'R', 'R'],
    ['T', 'T', 'T'],
    ['Y', 'Y', 'Y'],
    ['U', 'U', 'U'],
    ['I', 'I', 'I'],
    ['O', 'O', 'O'],
    ['P', 'P', 'P'],

    ['A', 'A', 'A'],
    ['S', 'S', 'S'],
    ['D', 'D', 'D'],
    ['F', 'F', 'F'],
    ['G', 'G', 'G'],
    ['H', 'H', 'H'],
    ['J', 'J', 'J'],
    ['K', 'K', 'K'],
    ['L', 'L', 'L'],
    ['↲', '\r', 'Enter'],

    ['⇧', 'code', 'ShiftLeft'],
    ['Z', 'Z', 'Z'],
    ['X', 'X', 'X'],
    ['C', 'C', 'C'],
    ['V', 'V', 'V'],
    ['B', 'B', 'B'],
    ['N', 'N', 'N'],
    ['M', 'M', 'M'],
    ['[', '[', '['],
    [']', ']', ']'],

    ['{', '{', '{'],
    ['}', '}', '}'],
    ['=', '=', 'Equals'],
    [' ', ' ', 'Space'],
    [' ', ' ', 'Space'],
    [' ', ' ', 'Space'],
    ['£', '£', 'Pound'],
    ['<', '<', 'Less'],
    ['>', '>', 'Greater'],
    ['~', '~', 'Question Mark'],

    ['⎈', 'code', 'ControlLeft'],
    ['⎇', 'code', 'AltLeft'],
    ['⇤', 'code', 'Home'],
    ['⇥', 'code', 'End'],
    ['⎀', 'code', 'Insert'],
    ['+', '+', 'Plus'],
    ['-', '-', 'Minus'],
    ['∗', '*', 'Multiply'],
    ['/', '/', 'Slash'],
    ['^', '^', 'Power'],
];

// if you hold the ALT key and press a character, you get a petscii.
// if you hold both: Shift+Alt you get another one.
// the Alt one is the shifted on the CBM,
// Shift+Alt simulated the CBM key.
// Returns 0 on error
function unicodeFromAltKeyPressInt(keyChar, withShift) {
    const pet2uni = {
        0x5C: 0x000000A3, // (*) pound sign !There's no backslash '\' in PETSCII!
        0x5E: 0x00002191, // (!) arrow up
        0x5F: 0x00002190, // (!) arrow left
        0x60: 0x00002501, // (!) box drawings light horizontal
        0x61: 0x00000061, // (*) a  - LOWERCASE - COMPATIBLE WITH ASCII - NOT PETSCI!
        0x62: 0x00000062, // (*) b
        0x63: 0x00000063, // (*) c
        0x64: 0x00000064, // (*) d
        0x65: 0x00000065, // (*) e
        0x66: 0x00000066, // (*) f
        0x67: 0x00000067, // (*) g
        0x68: 0x00000068, // (*) h
        0x69: 0x00000069, // (*) i
        0x6A: 0x0000006A, // (*) j
        0x6B: 0x0000006B, // (*) k
        0x6C: 0x0000006C, // (*) l
        0x6D: 0x0000006D, // (*) m
        0x6E: 0x0000006E, // (*) n
        0x6F: 0x0000006F, // (*) o
        0x70: 0x00000070, // (*) p
        0x71: 0x00000071, // (*) q
        0x72: 0x00000072, // (*) r
        0x73: 0x00000073, // (*) s
        0x74: 0x00000074, // (*) t
        0x75: 0x00000075, // (*) u
        0x76: 0x00000076, // (*) v
        0x77: 0x00000077, // (*) w
        0x78: 0x00000078, // (*) x
        0x79: 0x00000079, // (*) y
        0x7A: 0x0000007A, // (*) z  - END LOWERCASE - COMPATIBLE WITH ASCII - NOT PETSCI!
        0x7B: 0x0000253C, // (-) box drawings light vertical and horizontal
        0x7C: 0x0000e011, // (E) left half hatched
        0x7D: 0x00002503, // (!) box drawings heavy vertical
        0x7E: 0x000003C0, // (!) greek small letter pi
        0x7F: 0x000025E5, // (!) black upper right triangle
        0x80: 0x00000080, // (-) undefined
        0x81: 0x00000081, // (-) orange color switch
        0x82: 0x00000082, // (-) undefined
        0x83: 0x00000083, // (-) run
        0x84: 0x00000084, // (-) undefined
        0x85: 0x00000085, // (-) F1
        0x86: 0x00000086, // (-) F3
        0x87: 0x00000087, // (-) F5
        0x88: 0x00000088, // (-) F7
        0x89: 0x00000089, // (-) F2
        0x8A: 0x0000008A, // (-) F4
        0x8B: 0x0000008B, // (-) F6
        0x8C: 0x0000008C, // (-) F8
        0x8D: 0x0000008D, // (-) Shift+Return
        0x8E: 0x0000008E, // (-) Uppercase
        0x8F: 0x0000008F, // (-) undefined
        0x90: 0x00000090, // (-) black color
        0x91: 0x00000091, // (-) cursor up
        0x92: 0x00000092, // (-) turn off inverse colors
        0x93: 0x00000093, // (-) clear
        0x94: 0x00000094, // (-) insert
        0x95: 0x00000095, // (-) brown color
        0x96: 0x00000096, // (-) pink/light red color
        0x97: 0x00000097, // (-) dark gray color
        0x98: 0x00000098, // (-) medium gray color
        0x99: 0x00000099, // (-) light green color
        0x9A: 0x0000009A, // (-) light blue color
        0x9B: 0x0000009B, // (-) light gray color
        0x9C: 0x0000009C, // (-) purple color
        0x9D: 0x0000009D, // (-) cursor left
        0x9E: 0x0000009E, // (-) yellow color
        0x9F: 0x0000009F, // (-) cyan color
        0xA0: 0x000000A0, // (-) no breaking space
        0xA1: 0x0000258C, // (!) left half block
        0xA2: 0x00002584, // (!) lower half block
        0xA3: 0x000000A3, // (*) pound sign (compatible with ASCII!)
        0xA4: 0x00002581, // (!) lower one eighth block
        0xA5: 0x0000258E, // (!) left one quarter block  258F, // (!) left one eighth block
        0xA6: 0x00002592, // (!) medium shade
        0xA7: 0x0001FB87, // (!) RIGHT ONE QUARTER BLOCK
        0xA8: 0x0001FB8F, // (!) LOWER HALF MEDIUM SHADE
        0xA9: 0x000025E4, // (!) black upper left triangle
        0xAA: 0x0001FB87, // (!) RIGHT ONE QUARTER BLOCK
        0xAB: 0x00002523, // (!) box drawings heavy vertical and right
        0xAC: 0x00002597, // (!) black small square lower right
        0xAD: 0x00002517, // (!) box drawings heavy up and right
        0xAE: 0x00002513, // (!) box drawings heavy down and left
        0xAF: 0x00002582, // (!) lower one quarter block
        0xB0: 0x0000250F, // (!) box drawings heavy down and right
        0xB1: 0x0000253B, // (!) box drawings heavy up and horizontal
        0xB2: 0x00002533, // (!) box drawings heavy down and horizontal
        0xB3: 0x0000252B, // (!) box drawings heavy vertical and left
        0xB4: 0x0000258E, // (!) left one quarter block
        0xB5: 0x0000258D, // (!) left three eights block
        0xB6: 0x0001FB88, // (!) RIGHT THREE EIGHTHS BLOCK
        0xB7: 0x0001FB82, // (!) UPPER ONE QUARTER BLOCK
        0xB8: 0x0001FB83, // (!) UPPER THREE EIGHTHS BLOCK
        0xB9: 0x00002583, // (!) lower three eights block
        0xBA: 0x0001FB7F, // (!) bottom right corner
        0xBB: 0x00002596, // (!) black small square lower left
        0xBC: 0x0000259D, // (!) black small square upper rights
        0xBD: 0x0000251B, // (!) box drawings heavy up and left
        0xBE: 0x00002598, // (!) black small square upper left
        0xBF: 0x0000259A, // (!) two small black squares diagonal left to right
        0xC0: 0x00002501, // (!) box drawings light horizontal
        0xC1: 0x00002660, // (!) black spade suit
        0xC2: 0x00002758, // (!) LIGHT VERTICAL BAR
        0xC3: 0x00002501, // (!) BOX DRAWINGS HEAVY HORIZONTAL           1FB78, // (!) box drawings light horizontal
        0xC4: 0x0001FB77, // (!) box drawings light horizontal one quarter up
        0xC5: 0x0001FB76, // (!) box drawings light horizontal two quarters up (bad)
        0xC6: 0x0001FB7A, // (!) box drawings light horizontal one quarter down
        0xC7: 0x0001FB71, // (!) box drawings light vertical one quarter left
        0xC8: 0x0001FB74, // (!) box drawings light vertical one quarter right
        0xC9: 0x0000256E, // (!) box drawings light arc down and left
        0xCA: 0x00002570, // (!) box drawings light arc up and right
        0xCB: 0x0000256F, // (!) box drawings light arc up and left
        0xCC: 0x0001FB7C, // (!) bottom left corner
        0xCD: 0x00002572, // (!) box drawings light diagonal upper left to lower right
        0xCE: 0x00002571, // (!) box drawings light diagonal upper right to lower left
        0xCF: 0x0001FB7D, // (!) top left corner
        0xD0: 0x0001FB7E, // (!) top right corner
        0xD1: 0x000025CF, // (!) black circle
        0xD2: 0x0001FB7B, // (!) box drawings light horizontal two quarters down
        0xD3: 0x00002665, // (!) black heart suit
        0xD4: 0x0001FB70, // (!) box drawings light vertical two quarters left
        0xD5: 0x0000256D, // (!) box drawings light arc down and right
        0xD6: 0x00002573, // (!) box drawings light diagonal cross
        0xD7: 0x000025CB, // (!) donut
        0xD8: 0x00002663, // (!) black club suit
        0xD9: 0x0001FB75, // (!) box drawings light vertical two quarters right
        0xDA: 0x00002666, // (!) black diamond suit
        0xDB: 0x0000253C, // (!) box drawings light vertical and horizontal
        0xDC: 0x0001FB8C, // (!) LEFT HALF MEDIUM SHADE
        0xDD: 0x00002502, // (!) box drawings light vertical
        0xDE: 0x000003C0, // (!) greek small letter pi
        0xDF: 0X0001FB98, // (!) UPPER LEFT TO LOWER RIGHT FILL '\\'  --- from lowercase
        0xE0: 0x000000A0, // (!) no-break space
        0xE1: 0x0000258C, // (!) left half block
        0xE2: 0x00002584, // (!) lower half block
        0xE3: 0x00002594, // (!) upper one eighth block
        0xE4: 0x00002581, // (!) lower one eighth block
        0xE5: 0x0000258E, // (!) left one quarter block   /* 0xE5, */ 0x0000258F, // (!) left one eighth block
        0xE6: 0x00002592, // (!) medium shade
        0xE5: 0x0001FB87, // (!) RIGHT ONE QUARTER BLOCK           /* 0xE7, */ 0x00002595, // (!) right one eighth block
        0xE8: 0x0001FB8F, // (!) LOWER HALF MEDIUM SHADE
        0xE9: 0x0001fb99, // (!) UPPER RIGHT TO LOWER LEFT FILL '//'
        0xEA: 0x0001FB87, // (!) right one quarter block
        0xEB: 0x00002523, // (!) box drawings heavy vertical and right
        0xEC: 0x00002597, // (!) black small square lower right
        0xED: 0x00002517, // (!) box drawings heavy up and right
        0xEE: 0x00002513, // (!) box drawings heavy down and left
        0xEF: 0x00002582, // (!) lower one quarter block
        0xF0: 0x0000250F, // (!) box drawings heavy down and right
        0xF1: 0x0000253B, // (!) box drawings heavy up and horizontal
        0xF2: 0x00002533, // (!) box drawings heavy down and horizontal
        0xF3: 0x0000252B, // (!) box drawings heavy vertical and left
        0xF4: 0x0000258E, // (!) left one quarter block
        0xF5: 0x0000258D, // (!) left three eights block
        0xF6: 0x0001FB88, // (!) right three eights block
        0xF7: 0x0001FB82, // (!) upper one quarter block
        0xF8: 0x0001FB83, // (!) upper three eights block
        0xF9: 0x00002583, // (!) lower three eights block
        0xFA: 0x00002713, // (!) check mark  --- from lowercase
        0xFB: 0x00002596, // (!) black small square lower left
        0xFC: 0x0000259D, // (!) black small square upper right
        0xFD: 0x0000251b, // (!) box drawings heavy up and left
        0xFE: 0x00002598, // (!) black small square upper left
        0xFF: 0x00002592, //  (?) medium shade  --- from lowercase (should be inverted)
    };


    const puoundChar = '/'; // char('\xdf'); // German sz
    var c = keyChar.toLowerCase();

    var cp = 0;
    if (withShift) {
        var ix = 0;
        if (c >= 'a' && c <= 'z') {
            ix = c.codePointAt(0) - 0x61 + 0xc1;
        }
        if (c == '+') {
            ix = 0x7b;
        }
        if (c == '-') {
            ix = 0x7d;
        }
        if (c == '@') {
            ix = 0xba;
        }
        if (c == '*') {
            ix = 0x60;
        }
        if (c == puoundChar) {
            ix = 0xa9;
        }
        cp = pet2uni[ix];
    } else {
        const cbmmap = [
            ['a', 0xB0],
            ['b', 0xBF],
            ['c', 0xBC],
            ['d', 0xAC],
            ['e', 0xB1],
            ['f', 0xBB],
            ['g', 0xA5],
            ['h', 0xB4],
            ['i', 0xA2],
            ['j', 0xB5],
            ['k', 0xA1],
            ['l', 0xB6],
            ['m', 0xA7],
            ['n', 0xAA],
            ['o', 0xB9],
            ['p', 0xAF],
            ['q', 0xAB],
            ['r', 0xB2],
            ['s', 0xAE],
            ['t', 0xA3],
            ['u', 0xB8],
            ['v', 0xBE],
            ['w', 0xB3],
            ['x', 0xBD],
            ['y', 0xB7],
            ['z', 0xAD],
            ['+', 0x7f],
            ['-', 0xA6],
            ['@', 0x7c],
            ['*', 0x5F],
            [puoundChar, 0xA8],
        ];

        // GPT says +-@ is a6,dc,a4
        for (var i = 0; i < cbmmap.length; ++i) {
            if (cbmmap[i][0] == c) {
                cp = pet2uni[cbmmap[i][1]];
                break;
            }
        }
    }
    if (cp == undefined) {
        return 0;
    }


    // as of 2025-08-22,
    // no font supports LOWER HALF MEDIUM SHADE (U+1FB8F),
    // so we go for U + 2584 — LOWER HALF BLOCK on the keyboard.
    // BA67 does, however, support it.
    // if (cp == 0x0001fb8f) { cp = 0x00002584; }
    // stop! one font does: BA68.

    return cp;
}



var altKeys = [];
var shiftedAltKeys = [];
for (var i = 0; i < keys.length; ++i) {
    var lab = keys[i][0];
    var key = keys[i][1];
    var cod = keys[i][2];
    if (key != 'code') {
        var cp = unicodeFromAltKeyPressInt(key, false);
        if (cp != 0) {
            lab = String.fromCodePoint(cp);
            cod = lab;
        }
        // send 'a' with Alt -> BA67 will convert to PETSCII
        altKeys.push([lab, key, cod]);

        var cp = unicodeFromAltKeyPressInt(key, true);
        if (cp != 0) {
            lab = String.fromCodePoint(cp);
            cod = lab;
        }
        shiftedAltKeys.push([lab, key, cod]);
    } else {
        altKeys.push([lab, key, cod]);
        shiftedAltKeys.push([lab, key, cod]);
    }
}

var currentKeys = keys;

var keyboardDiv = document.getElementById('keyboard');
// if (!keyboardDiv) {
//     keyboardDiv = document.createElement('div');
//     keyboardDiv.setAttribute("id", "keyboard");
//     keyboardDiv.classList.add('keyboard');
// }

function buildKeys() {
    var fc = keyboardDiv.firstChild;

    while (fc) {
        keyboardDiv.removeChild(fc);
        fc = keyboardDiv.firstChild;
    }
    // Build keyboard UI
    currentKeys = keys;
    if (altActive) {
        if (shiftActive) {
            currentKeys = shiftedAltKeys;
        } else {
            currentKeys = altKeys;
        }
    } else if (shiftActive) {
        currentKeys = shiftedKeys;
    }

    currentKeys.forEach(k => {
        const btn = document.createElement('button');
        btn.classList.add('key');
        const label = k[0];
        btn.textContent = label;
        var tip = k[2];
        if (tip.substr(0, 3) == 'Key') { tip = tip.substr(3); }
        btn.setAttribute('title', tip);
        btn.addEventListener('click', () => handleKeyPress(k));

        if (shiftActive && tip == 'ShiftLeft') { toggleButton(label, true); }
        if (ctrlActive && tip == 'ControlLeft') { toggleButton(label, true); }
        if (altActive && tip == 'AltLeft') { toggleButton(label, true); }

        keyboardDiv.appendChild(btn);
    });
    if (shiftActive) {
        toggleButton('⇧', shiftActive);
    }
    if (altActive) {
        toggleButton('⎇', altActive);
    }
    if (ctrlActive) {
        toggleButton('⎈', ctrlActive);
    }
}
buildKeys();

function simulateKey(code, shiftActive, altActive, ctrlActive) {
    const key = ''; // printable chacacter
    const down = new KeyboardEvent('keydown', {
        'key': key,
        'code': code,
        'ctrlKey': ctrlActive,
        'shiftKey': shiftActive,
        'altKey': altActive,
        'bubbles': true,
    });
    canvas.dispatchEvent(down);

    const up = new KeyboardEvent('keyup', {
        'key': key,
        'code': code,
        'ctrlKey': ctrlActive,
        'shiftKey': shiftActive,
        'altKey': altActive,
        'bubbles': true,
    });
    canvas.dispatchEvent(up);
}

function sendText(char, shiftActive, altActive, ctrlActive) {
    // Direct SDL_TEXTINPUT injection
    if (typeof Module !== 'undefined' && Module.ccall) {
        console.log("SDL_TEXTINPUT pushed:", char);
        Module.ccall(
            'ba67_push_sdl_textinput', null,
            ['string', 'bool', 'bool', 'bool'],
            [char, shiftActive, altActive, ctrlActive]
        );
    } else {
        console.log("SDL_TEXTINPUT failed for:", char);
    }
}

function handleKeyPress(keyDef) {
    const label = keyDef[0];
    const key = keyDef[1];
    const code = keyDef[2];
    // const [label, key, code] = keyDef;

    if (code === 'ShiftLeft') {
        shiftActive = !shiftActive;
        toggleButton(label, shiftActive);
        buildKeys();
        return;
    }
    if (code === 'AltLeft') {
        altActive = !altActive;
        toggleButton(label, altActive);
        buildKeys();
        return;
    }
    if (code === 'ControlLeft') {
        ctrlActive = !ctrlActive;
        toggleButton(label, ctrlActive);
        buildKeys();
        return;
    }


    if (key == ('code')) {
        // works for arrow keys etc.
        simulateKey(code, shiftActive, altActive, ctrlActive);
    } else {
        // required for real text input
        // simulateKey(code, shiftActive, altActive, ctrlActive);
        let outputChar = key;
        sendText(outputChar, shiftActive, altActive, ctrlActive);
    }
}

function toggleButton(label, state) {
    [...keyboardDiv.children].forEach(btn => {
        if (btn.textContent === label) {
            btn.classList.toggle('active', state);
        }
    });
}

function updateKeyboardVisibility() {
    // const div = document.getElementById('keyboard');
    const winH = window.innerHeight;
    const divH = keyboardDiv.scrollHeight;

    if (divH > winH * 0.48) {
        // hide (collapse)
        keyboardDiv.classList.remove("visible"); // hide
    } else {
        // show (expand to its content height)
        keyboardDiv.classList.add("visible"); // show
    }
}
// Run on load and on resize
window.addEventListener("load", updateKeyboardVisibility);
window.addEventListener("resize", updateKeyboardVisibility);
