// UTF-8 äöüß

// Modifier states
let shiftActive = false;
let altActive = false;
let ctrlActive = false;

// Key definition array: [label, key, code]
// key-codes: https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values
const keys = [
    //label key      code            LABEL KEY       CODE
    // ------------------------------|-------------------------------- 
    ['ES', '\u001b', '␛', 'ES', '\u0027', '␛'],
    ['↑', 'code', 'ArrowUp', '↑', 'code', 'ArrowUp'],
    ['↓', 'code', 'ArrowDown', '↓', 'code', 'ArrowDown'],
    ['←', 'code', 'ArrowLeft', '←', 'code', 'ArrowLeft'],
    ['→', 'code', 'ArrowRight', '→', 'code', 'ArrowRight'],
    ['F1', 'code', 'F1', 'F2', 'code', 'F2'],
    ['F3', 'code', 'F3', 'F4', 'code', 'F4'],
    ['F5', 'code', 'F5', 'F6', 'code', 'F6'],
    ['F7', 'code', 'F7', 'F8', 'code', 'F8'],
    ['⌫', '\b', 'Backspace', '⌫', '\b', 'Backspace'],
    // ------------------------------|-------------------------------- 
    ['1', '1', '1', '!', '!', 'Exclamation'],
    ['2', '2', '2', '"', '"', 'Quotes'],
    ['3', '3', '3', '#', '#', 'Hash'],
    ['4', '4', '4', '$', '$', 'Dollar'],
    ['5', '5', '5', '%', '%', 'Percent'],
    ['6', '6', '6', '&', '&', 'Ampersand'],
    ['7', '7', '7', "/", "/", 'Divide'],
    ['8', '8', '8', '(', '(', '('],
    ['9', '9', '9', ')', ')', ')'],
    ['0', '0', '0', '0', '0', '0'],
    // ------------------------------|-------------------------------- 
    ['q', 'q', 'Q', 'Q', 'Q', 'Q'],
    ['w', 'w', 'W', 'W', 'W', 'W'],
    ['e', 'e', 'E', 'E', 'E', 'E'],
    ['r', 'r', 'R', 'R', 'R', 'R'],
    ['t', 't', 'T', 'T', 'T', 'T'],
    ['y', 'y', 'Y', 'Y', 'Y', 'Y'],
    ['u', 'u', 'U', 'U', 'U', 'U'],
    ['i', 'i', 'I', 'I', 'I', 'I'],
    ['o', 'o', 'O', 'O', 'O', 'O'],
    ['p', 'p', 'P', 'P', 'P', 'P'],
    // ------------------------------|-------------------------------- 
    ['a', 'a', 'A', 'A', 'A', 'A'],
    ['s', 's', 'S', 'S', 'S', 'S'],
    ['d', 'd', 'D', 'D', 'D', 'D'],
    ['f', 'f', 'F', 'F', 'F', 'F'],
    ['g', 'g', 'G', 'G', 'G', 'G'],
    ['h', 'h', 'H', 'H', 'H', 'H'],
    ['j', 'j', 'J', 'J', 'J', 'J'],
    ['k', 'k', 'K', 'K', 'K', 'K'],
    ['l', 'l', 'L', 'L', 'L', 'L'],
    ['↲', '\r', 'Enter', '↲', '\r', 'Enter'],
    // ------------------------------|-------------------------------- 
    ['⇧', 'code   ', 'ShiftLeft', '⇧', 'code   ', 'ShiftLeft'],
    ['z', 'z', 'Z', 'Z', 'Z', 'Z'],
    ['x', 'x', 'X', 'X', 'X', 'X'],
    ['c', 'c', 'C', 'C', 'C', 'C'],
    ['v', 'v', 'V', 'V', 'V', 'V'],
    ['b', 'b', 'B', 'B', 'B', 'B'],
    ['n', 'n', 'N', 'N', 'N', 'N'],
    ['m', 'm', 'M', 'M', 'M', 'M'],
    [':', ':', 'Colon', '[', '[', 'Colon'],
    [';', ';', 'Semicolon', ']', ']', 'Semicolon'],
    // ------------------------------|-------------------------------- 
    ['<', '<', 'Less', '{', '{', '{'],
    ['>', '>', 'Greater', '}', '}', '}'],
    ['=', '=', 'Equals', '=', '=', 'Equals'],
    [' ', ' ', 'Space', ' ', ' ', 'Space'],
    [' ', ' ', 'Space', ' ', ' ', 'Space'],
    [' ', ' ', 'Space', ' ', ' ', 'Space'],
    ['@', '@', 'At', '£', '£', 'Pound'],
    [',', ',', 'Comma', '<', '<', 'Less'],
    ['.', '.', 'Period', '>', '>', 'Greater'],
    ['?', '?', 'Question Mark', '~', '~', 'Question Mark'],
    // ------------------------------|-------------------------------- 
    ['⎈', 'code', 'ControlLeft', '⎈', 'code', 'ControlLeft'],
    ['⎇', 'code', 'AltLeft', '⎇', 'code', 'AltLeft'],
    ['⇤', 'code', 'Home', '⇤', 'code', 'Home'],
    ['⇥', 'code', 'End', '⇥', 'code', 'End'],
    ['⎀', 'code', 'Insert', '⎀', 'code', 'Insert'],
    ['+', '+', 'Plus', '+', '+', 'Plus'],
    ['-', '-', 'Minus', '-', '-', 'Minus'],
    ['∗', '*', 'Multiply', '∗', '*', 'Multiply'],
    ['/', '/', 'Slash', '/', '/', 'Slash'],
    ['^', '^', 'Power', '^', '^', 'Power'],
];

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
    keys.forEach(k => {
        const btn = document.createElement('button');
        btn.classList.add('key');
        const offset = shiftActive ? 3 : 0;
        const label = k[0 + offset];
        btn.textContent = label;
        var tip = k[2 + offset];
        if (tip.substr(0, 3) == 'Key') { tip = tip.substr(3); }
        btn.setAttribute('title', tip);
        btn.addEventListener('click', () => handleKeyPress(k));

        if (shiftActive && tip == 'ShiftLeft') { toggleButton(label, true); }
        if (ctrlActive && tip == 'ControlLeft') { toggleButton(label, true); }
        if (altActive && tip == 'AltLeft') { toggleButton(label, true); }

        keyboardDiv.appendChild(btn);
    });
    if (shiftActive) {
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
    const offset = shiftActive ? 3 : 0;
    const label = keyDef[0 + offset];
    const key = keyDef[1 + offset];
    const code = keyDef[2 + offset];
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
        return;
    }
    if (code === 'ControlLeft') {
        ctrlActive = !ctrlActive;
        toggleButton(label, ctrlActive);
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
    if (shiftActive) {
        shiftActive = false;
        buildKeys();
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
