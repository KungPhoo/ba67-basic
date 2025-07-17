#include "petscii.h"
#include <unordered_map>

char32_t PETSCII::toUnicode(uint8_t petscii) {
    // Mapping of PETSCII.BA67 to Unicode
    // See petscii-mapping.png for what each 'petscii' maps to
    // There:
    //        blue:   character is from upper case font
    //        orange: character is ascii -> not compatible with upper nor lowercase PETSCII
    //        green:  character is taken from lower case font

    // legend:
    // (-) Unicode equals PETSCII
    // (!) Unicode code point differs from PETSCII code
    // (*) Character is neither upper nor lowercase PETSCII
    // (?) No Unicode pendant was found. The PETSCII code is used

    static char32_t petsciiMapping[256]
        = {
              /* 0x00, */ 0x00000000, // (-) undefined
              /* 0x01, */ 0x00000001, // (-) undefined
              /* 0x02, */ 0x00000002, // (-) undefined
              /* 0x03, */ 0x00000003, // (-) ETX - End of text
              /* 0x04, */ 0x00000004, // (-) undefined
              /* 0x05, */ 0x00000005, // (-) white color
              /* 0x06, */ 0x00000006, // (-) undefined
              /* 0x07, */ 0x00000007, // (-) undefined
              /* 0x08, */ 0x00000008, // (-) disable Shift+C=
              /* 0x09, */ 0x00000009, // (-) enable  Shift+C=
              /* 0x0A, */ 0x0000000A, // (-) '\n' (ASCII)
              /* 0x0B, */ 0x0000000B, // (-) undefined
              /* 0x0C, */ 0x0000000C, // (-) undefined
              /* 0x0D, */ 0x0000000D, // (-) '\r'
              /* 0x0E, */ 0x0000000E, // (-) lowercase
              /* 0x0F, */ 0x0000000F, // (-) undefined

              /* 0x10, */ 0x00000010, // (-) undefined
              /* 0x11, */ 0x00000011, // (-) cursor down
              /* 0x12, */ 0x00000012, // (-) inverse colors
              /* 0x13, */ 0x00000013, // (-) home
              /* 0x14, */ 0x00000014, // (-) delete
              /* 0x15, */ 0x00000015, // (-) undefined
              /* 0x16, */ 0x00000016, // (-) undefined
              /* 0x17, */ 0x00000017, // (-) undefined
              /* 0x18, */ 0x00000018, // (-) undefined
              /* 0x19, */ 0x00000019, // (-) undefined
              /* 0x1A, */ 0x0000001A, // (-) undefined
              /* 0x1B, */ 0x0000001B, // (-) undefined
              /* 0x1C, */ 0x0000001C, // (-) red color
              /* 0x1D, */ 0x0000001D, // (-) cursor right
              /* 0x1E, */ 0x0000001E, // (-) green color
              /* 0x1F, */ 0x0000001F, // (-) blue color

              /* 0x20, */ 0x00000020, // (-) Space
              /* 0x21, */ 0x00000021, // (-) !
              /* 0x22, */ 0x00000022, // (-) "
              /* 0x23, */ 0x00000023, // (-) #
              /* 0x24, */ 0x00000024, // (-) $
              /* 0x25, */ 0x00000025, // (-) %
              /* 0x26, */ 0x00000026, // (-) &
              /* 0x27, */ 0x00000027, // (-) '
              /* 0x28, */ 0x00000028, // (-) (
              /* 0x29, */ 0x00000029, // (-) )
              /* 0x2A, */ 0x0000002A, // (-) *
              /* 0x2B, */ 0x0000002B, // (-) +
              /* 0x2C, */ 0x0000002C, // (-) ,
              /* 0x2D, */ 0x0000002D, // (-) -
              /* 0x2E, */ 0x0000002E, // (-) .
              /* 0x2F, */ 0x0000002F, // (-) '/'

              /* 0x30, */ 0x00000030, // (-) 0
              /* 0x31, */ 0x00000031, // (-) 1
              /* 0x32, */ 0x00000032, // (-) 2
              /* 0x33, */ 0x00000033, // (-) 3
              /* 0x34, */ 0x00000034, // (-) 4
              /* 0x35, */ 0x00000035, // (-) 5
              /* 0x36, */ 0x00000036, // (-) 6
              /* 0x37, */ 0x00000037, // (-) 7
              /* 0x38, */ 0x00000038, // (-) 8
              /* 0x39, */ 0x00000039, // (-) 9
              /* 0x3A, */ 0x0000003A, // (-) :
              /* 0x3B, */ 0x0000003B, // (-) ;
              /* 0x3C, */ 0x0000003C, // (-) <
              /* 0x3D, */ 0x0000003D, // (-) =
              /* 0x3E, */ 0x0000003E, // (-) >
              /* 0x3F, */ 0x0000003F, // (-) ?

              /* 0x40, */ 0x00000040, // (-) @
              /* 0x41, */ 0x00000041, // (-) A
              /* 0x42, */ 0x00000042, // (-) B
              /* 0x43, */ 0x00000043, // (-) C
              /* 0x44, */ 0x00000044, // (-) D
              /* 0x45, */ 0x00000045, // (-) E
              /* 0x46, */ 0x00000046, // (-) F
              /* 0x47, */ 0x00000047, // (-) G
              /* 0x48, */ 0x00000048, // (-) H
              /* 0x49, */ 0x00000049, // (-) I
              /* 0x4A, */ 0x0000004A, // (-) J
              /* 0x4B, */ 0x0000004B, // (-) K
              /* 0x4C, */ 0x0000004C, // (-) L
              /* 0x4D, */ 0x0000004D, // (-) M
              /* 0x4E, */ 0x0000004E, // (-) N
              /* 0x4F, */ 0x0000004F, // (-) O

              /* 0x50, */ 0x00000050, // (-) P
              /* 0x51, */ 0x00000051, // (-) Q
              /* 0x52, */ 0x00000052, // (-) R
              /* 0x53, */ 0x00000053, // (-) S
              /* 0x54, */ 0x00000054, // (-) T
              /* 0x55, */ 0x00000055, // (-) U
              /* 0x56, */ 0x00000056, // (-) V
              /* 0x57, */ 0x00000057, // (-) W
              /* 0x58, */ 0x00000058, // (-) X
              /* 0x59, */ 0x00000059, // (-) Y
              /* 0x5A, */ 0x0000005A, // (-) Z
              /* 0x5B, */ 0x0000005B, // (-) [
              /* 0x5C, */ 0x000000A3, // (*) pound sign !There's no backslash '\' in PETSCII!
              /* 0x5D, */ 0x0000005D, // (-) ]
              /* 0x5E, */ 0x00002191, // (!) arrow up
              /* 0x5F, */ 0x00002190, // (!) arrow left

              /* 0x60, */ 0x00002501, // (!) box drawings light horizontal
              /* 0x61, */ 0x00000061, // (*) a  - LOWERCASE - COMPATIBLE WITH ASCII - NOT PETSCI!
              /* 0x62, */ 0x00000062, // (*) b
              /* 0x63, */ 0x00000063, // (*) c
              /* 0x64, */ 0x00000064, // (*) d
              /* 0x65, */ 0x00000065, // (*) e
              /* 0x66, */ 0x00000066, // (*) f
              /* 0x67, */ 0x00000067, // (*) g
              /* 0x68, */ 0x00000068, // (*) h
              /* 0x69, */ 0x00000069, // (*) i
              /* 0x6A, */ 0x0000006A, // (*) j
              /* 0x6B, */ 0x0000006B, // (*) k
              /* 0x6C, */ 0x0000006C, // (*) l
              /* 0x6D, */ 0x0000006D, // (*) m
              /* 0x6E, */ 0x0000006E, // (*) n
              /* 0x6F, */ 0x0000006F, // (*) o

              /* 0x70, */ 0x00000070, // (*) p
              /* 0x71, */ 0x00000071, // (*) q
              /* 0x72, */ 0x00000072, // (*) r
              /* 0x73, */ 0x00000073, // (*) s
              /* 0x74, */ 0x00000074, // (*) t
              /* 0x75, */ 0x00000075, // (*) u
              /* 0x76, */ 0x00000076, // (*) v
              /* 0x77, */ 0x00000077, // (*) w
              /* 0x78, */ 0x00000078, // (*) x
              /* 0x79, */ 0x00000079, // (*) y
              /* 0x7A, */ 0x0000007A, // (*) z  - END LOWERCASE - COMPATIBLE WITH ASCII - NOT PETSCI!
              /* 0x7B, */ 0x0000253C, // (-) box drawings light vertical and horizontal
              /* 0x7C, */ 0x0000e011, // (E) left half hatched
              /* 0x7D, */ 0x00002503, // (!) box drawings heavy vertical
              /* 0x7E, */ 0x000003C0, // (!) greek small letter pi
              /* 0x7F, */ 0x000025E5, // (!) black upper right triangle

              /* 0x80, */ 0x00000080, // (-) undefined
              /* 0x81, */ 0x00000081, // (-) orange color switch
              /* 0x82, */ 0x00000082, // (-) undefined
              /* 0x83, */ 0x00000083, // (-) run
              /* 0x84, */ 0x00000084, // (-) undefined
              /* 0x85, */ 0x00000085, // (-) F1
              /* 0x86, */ 0x00000086, // (-) F3
              /* 0x87, */ 0x00000087, // (-) F5
              /* 0x88, */ 0x00000088, // (-) F7
              /* 0x89, */ 0x00000089, // (-) F2
              /* 0x8A, */ 0x0000008A, // (-) F4
              /* 0x8B, */ 0x0000008B, // (-) F6
              /* 0x8C, */ 0x0000008C, // (-) F8
              /* 0x8D, */ 0x0000008D, // (-) Shift+Return
              /* 0x8E, */ 0x0000008E, // (-) Uppercase
              /* 0x8F, */ 0x0000008F, // (-) undefined

              /* 0x90, */ 0x00000090, // (-) black color
              /* 0x91, */ 0x00000091, // (-) cursor up
              /* 0x92, */ 0x00000092, // (-) turn off inverse colors
              /* 0x93, */ 0x00000093, // (-) clear
              /* 0x94, */ 0x00000094, // (-) insert
              /* 0x95, */ 0x00000095, // (-) brown color
              /* 0x96, */ 0x00000096, // (-) pink/light red color
              /* 0x97, */ 0x00000097, // (-) dark gray color
              /* 0x98, */ 0x00000098, // (-) medium gray color
              /* 0x99, */ 0x00000099, // (-) light green color
              /* 0x9A, */ 0x0000009A, // (-) light blue color
              /* 0x9B, */ 0x0000009B, // (-) light gray color
              /* 0x9C, */ 0x0000009C, // (-) purple color
              /* 0x9D, */ 0x0000009D, // (-) cursor left
              /* 0x9E, */ 0x0000009E, // (-) yellow color
              /* 0x9F, */ 0x0000009F, // (-) cyan color

              /* 0xA0, */ 0x000000A0, // (-) no breaking space
              /* 0xA1, */ 0x0000258C, // (!) left half block
              /* 0xA2, */ 0x00002584, // (!) lower half block
              /* 0xA3, */ 0x000000A3, // (*) pound sign (compatible with ASCII!)
              /* 0xA4, */ 0x00002581, // (!) lower one eighth block
              /* 0xA5, */ 0x0000258E, // (!) left one quarter block  258F, // (!) left one eighth block
              /* 0xA6, */ 0x00002592, // (!) medium shade
              /* 0xA7, */ 0x0001FB87, // (!) RIGHT ONE QUARTER BLOCK
              /* 0xA8, */ 0x0001FB8F, // (!) LOWER HALF MEDIUM SHADE
              /* 0xA9, */ 0x000025E4, // (!) black upper left triangle
              /* 0xAA, */ 0x0001FB87, // (!) RIGHT ONE QUARTER BLOCK
              /* 0xAB, */ 0x00002523, // (!) box drawings heavy vertical and right
              /* 0xAC, */ 0x00002597, // (!) black small square lower right
              /* 0xAD, */ 0x00002517, // (!) box drawings heavy up and right
              /* 0xAE, */ 0x00002513, // (!) box drawings heavy down and left
              /* 0xAF, */ 0x00002582, // (!) lower one quarter block

              /* 0xB0, */ 0x0000250F, // (!) box drawings heavy down and right
              /* 0xB1, */ 0x0000253B, // (!) box drawings heavy up and horizontal
              /* 0xB2, */ 0x00002533, // (!) box drawings heavy down and horizontal
              /* 0xB3, */ 0x0000252B, // (!) box drawings heavy vertical and left
              /* 0xB4, */ 0x0000258E, // (!) left one quarter block
              /* 0xB5, */ 0x0000258D, // (!) left three eights block
              /* 0xB6, */ 0x0001FB88, // (!) RIGHT THREE EIGHTHS BLOCK
              /* 0xB7, */ 0x0001FB82, // (!) UPPER ONE QUARTER BLOCK
              /* 0xB8, */ 0x0001FB83, // (!) UPPER THREE EIGHTHS BLOCK
              /* 0xB9, */ 0x00002583, // (!) lower three eights block
              /* 0xBA, */ 0x0001FB7F, // (!) bottom right corner
              /* 0xBB, */ 0x00002596, // (!) black small square lower left
              /* 0xBC, */ 0x0000259D, // (!) black small square upper rights
              /* 0xBD, */ 0x0000251B, // (!) box drawings heavy up and left
              /* 0xBE, */ 0x00002598, // (!) black small square upper left
              /* 0xBF, */ 0x0000259A, // (!) two small black squares diagonal left to right

              /* 0xC0, */ 0x00002501, // (!) box drawings light horizontal
              /* 0xC1, */ 0x00002660, // (!) black spade suit
              /* 0xC2, */ 0x00002758, // (!) LIGHT VERTICAL BAR
              /* 0xC3, */ 0x00002501, // (!) BOX DRAWINGS HEAVY HORIZONTAL           1FB78, // (!) box drawings light horizontal
              /* 0xC4, */ 0x0001FB77, // (!) box drawings light horizontal one quarter up
              /* 0xC5, */ 0x0001FB76, // (!) box drawings light horizontal two quarters up (bad)
              /* 0xC6, */ 0x0001FB7A, // (!) box drawings light horizontal one quarter down
              /* 0xC7, */ 0x0001FB71, // (!) box drawings light vertical one quarter left
              /* 0xC8, */ 0x0001FB74, // (!) box drawings light vertical one quarter right
              /* 0xC9, */ 0x0000256E, // (!) box drawings light arc down and left
              /* 0xCA, */ 0x00002570, // (!) box drawings light arc up and right
              /* 0xCB, */ 0x0000256F, // (!) box drawings light arc up and left
              /* 0xCC, */ 0x0001FB7C, // (!) bottom left corner
              /* 0xCD, */ 0x00002572, // (!) box drawings light diagonal upper left to lower right
              /* 0xCE, */ 0x00002571, // (!) box drawings light diagonal upper right to lower left
              /* 0xCF, */ 0x0001FB7D, // (!) top left corner

              /* 0xD0, */ 0x0001FB7E, // (!) top right corner
              /* 0xD1, */ 0x000025CF, // (!) black circle
              /* 0xD2, */ 0x0001FB7B, // (!) box drawings light horizontal two quarters down
              /* 0xD3, */ 0x00002665, // (!) black heart suit
              /* 0xD4, */ 0x0001FB70, // (!) box drawings light vertical two quarters left
              /* 0xD5, */ 0x0000256D, // (!) box drawings light arc down and right
              /* 0xD6, */ 0x00002573, // (!) box drawings light diagonal cross
              /* 0xD7, */ 0x000025CB, // (!) donut
              /* 0xD8, */ 0x00002663, // (!) black club suit
              /* 0xD9, */ 0x0001FB75, // (!) box drawings light vertical two quarters right
              /* 0xDA, */ 0x00002666, // (!) black diamond suit
              /* 0xDB, */ 0x0000253C, // (!) box drawings light vertical and horizontal
              /* 0xDC, */ 0x0001FB8C, // (!) LEFT HALF MEDIUM SHADE
              /* 0xDD, */ 0x00002502, // (!) box drawings light vertical
              /* 0xDE, */ 0x000003C0, // (!) greek small letter pi
              /* 0xDF, */ 0X0001FB98, // (!) UPPER LEFT TO LOWER RIGHT FILL '\\'  --- from lowercase

              /* 0xE0, */ 0x000000A0, // (!) no-break space
              /* 0xE1, */ 0x0000258C, // (!) left half block
              /* 0xE2, */ 0x00002584, // (!) lower half block
              /* 0xE3, */ 0x00002594, // (!) upper one eighth block
              /* 0xE4, */ 0x00002581, // (!) lower one eighth block
              /* 0xE5, */ 0x0000258E, // (!) left one quarter block   /* 0xE5, */ 0x0000258F, // (!) left one eighth block
              /* 0xE6, */ 0x00002592, // (!) medium shade
              /* 0xE5, */ 0x0001FB87, // (!) RIGHT ONE QUARTER BLOCK           /* 0xE7, */ 0x00002595, // (!) right one eighth block
              /* 0xE8, */ 0x0001FB8F, // (!) LOWER HALF MEDIUM SHADE
              /* 0xE9, */ 0x0001fb99, // (!) UPPER RIGHT TO LOWER LEFT FILL '//'
              /* 0xEA, */ 0x0001FB87, // (!) right one quarter block
              /* 0xEB, */ 0x00002523, // (!) box drawings heavy vertical and right
              /* 0xEC, */ 0x00002597, // (!) black small square lower right
              /* 0xED, */ 0x00002517, // (!) box drawings heavy up and right
              /* 0xEE, */ 0x00002513, // (!) box drawings heavy down and left
              /* 0xEF, */ 0x00002582, // (!) lower one quarter block

              /* 0xF0, */ 0x0000250F, // (!) box drawings heavy down and right
              /* 0xF1, */ 0x0000253B, // (!) box drawings heavy up and horizontal
              /* 0xF2, */ 0x00002533, // (!) box drawings heavy down and horizontal
              /* 0xF3, */ 0x0000252B, // (!) box drawings heavy vertical and left
              /* 0xF4, */ 0x0000258E, // (!) left one quarter block
              /* 0xF5, */ 0x0000258D, // (!) left three eights block
              /* 0xF6, */ 0x0001FB88, // (!) right three eights block
              /* 0xF7, */ 0x0001FB82, // (!) upper one quarter block
              /* 0xF8, */ 0x0001FB83, // (!) upper three eights block
              /* 0xF9, */ 0x00002583, // (!) lower three eights block
              /* 0xFA, */ 0x00002713, // (!) check mark  --- from lowercase
              /* 0xFB, */ 0x00002596, // (!) black small square lower left
              /* 0xFC, */ 0x0000259D, // (!) black small square upper right
              /* 0xFD, */ 0x0000251b, // (!) box drawings heavy up and left
              /* 0xFE, */ 0x00002598, // (!) black small square upper left
              /* 0xFF, */ 0x00002592 //  (?) medium shade  --- from lowercase (should be inverted)
          };

    return petsciiMapping[petscii];
}

uint8_t PETSCII::fromUnicode(char32_t c, uint8_t fallback) {
    static std::unordered_map<char32_t, uint8_t> mapping;
    if (mapping.empty()) {
        for (size_t i = 0; i < 0x00ff; ++i) {
            mapping[toUnicode(uint8_t(i & 0xff))] = uint8_t(i & 0xff);
        }
    }
    auto it = mapping.find(c);
    if (it == mapping.end()) {
        return fallback;
    }
    return it->second;
};