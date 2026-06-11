#!/usr/bin/env python3
"""
Neuromancer text data extractor (comprehensive).
Extracts ALL textual content from the game's .DAT resource files:
  - Room BIH files (R1-R58): room descriptions and NPC dialog
  - Database BIH files (DB0-DB38): cyberspace "website" text
  - Special BIH files: NEWS, PAXBBS, COPEN, PSYCHO, SEA, IRS, etc.
  - TXH files: FTUSER, PAXCODES, ROMCON, AITALK, ENDGAME, ICE

Based on resource tables from:
  - Javamancer BihResource.java / TxhResource.java
  - Reuromancer resources_lists.c

Output: JSON with all extracted text, organized by category.
"""

import struct
import json
import sys
import os
from collections import OrderedDict


# ====================================================================
# Resource tables
# Format: (file_index, name, offset, compressed_size)
# file_index: 0 = NEURO1.DAT, 1 = NEURO2.DAT
# ====================================================================

# Room + special BIH resources (from resources_lists.c + BihResource.java)
RES_BIH = [
    # NEURO1.DAT - Room files
    (0, "R1.BIH",            0,       0x5EE),
    (0, "R2.BIH",       0x1C19,    0xC7),
    (0, "R3.BIH",       0x3CD3,   0x8F7),  # Fixed: was 0x87F
    (0, "R4.BIH",       0x60D0,   0x47A),
    (0, "R5.BIH",       0x911F,    0x2D),
    (0, "R6.BIH",       0xA54B,   0x7EB),
    (0, "R7.BIH",       0xCAAE,   0x2A7),
    (0, "R8.BIH",       0xE827,   0x9A4),
    (0, "R9.BIH",      0x10C02,   0x4A4),
    (0, "R10.BIH",     0x1319E,   0x3C3),
    (0, "R11.BIH",     0x145B2,   0x79C),
    (0, "R12.BIH",     0x1629C,   0x94C),
    (0, "R13.BIH",     0x19185,    0x2D),
    (0, "R14.BIH",     0x1AC51,    0x2D),
    (0, "R15.BIH",     0x1CA7C,    0x2D),
    (0, "R16.BIH",     0x1E7B4,    0x2D),
    (0, "R17.BIH",     0x204A1,    0xB9),
    (0, "R18.BIH",     0x2206D,    0x2D),
    (0, "R19.BIH",     0x237CA,   0x499),
    (0, "R20.BIH",     0x26DCC,   0x64A),
    (0, "R21.BIH",     0x28A62,    0x26),
    (0, "R22.BIH",     0x294DB,   0x293),
    (0, "R23.BIH",     0x29EA7,   0x817),
    (0, "R24.BIH",     0x2C156,   0x583),
    (0, "R25.BIH",     0x2F052,   0x3E4),
    (0, "R26.BIH",     0x2FF8F,   0x813),
    (0, "R27.BIH",     0x33100,   0x7EB),
    (0, "R28.BIH",     0x34E5B,   0x412),
    (0, "CORNERS.BIH", 0x3526D,    0x21),
    (0, "ROOMPOS.BIH", 0x3528E,   0x336),
    # NEURO1.DAT - TXH (listed here for completeness but processed separately)
    (0, "FTUSER.TXH",  0x3C2E3,   0x362),

    # NEURO2.DAT - Database + special BIH files (from BihResource.java)
    (1, "COPEN0.BIH",    0x00000,  0x3D2),
    (1, "COPEN1.BIH",    0x003D2,  0x39B),
    (1, "DB0.BIH",       0x0076D, 0x09F2),  # RegularFellowsDatabase
    (1, "DB1.BIH",       0x0115F, 0x0EC5),  # ConsumerReviewDatabase
    (1, "DB2.BIH",       0x02024, 0x0514),  # AsanosDatabase
    (1, "DB3.BIH",       0x02538, 0x0920),  # WorldChessDatabase
    (1, "DB4.BIH",       0x02E58, 0x0665),  # CheapHotelDatabase
    (1, "DB5.BIH",       0x034BD, 0x076D),  # PsychologistDatabase
    (1, "DB6.BIH",       0x03C2A, 0x0811),  # PantherModernsDatabase
    (1, "DB7.BIH",       0x0443B, 0x0A02),  # IRSDatabase
    (1, "DB8.BIH",       0x04E3D, 0x089D),  # FujiElectricDatabase
    (1, "DB9.BIH",       0x056DA, 0x0129),  # ChibaCityPoliceDatabase
    (1, "DB10.BIH",      0x05803, 0x0768),  # HitachiBiotechDatabase
    (1, "DB11.BIH",      0x05F6B, 0x0BB5),  # CopenhagenUniversityDatabase
    (1, "DB12.BIH",      0x06B20, 0x0880),  # SoftwareEnforcementDatabase
    (1, "DB13.BIH",      0x073A0, 0x0778),  # FreeMatrixDatabase
    (1, "DB14.BIH",      0x07B18, 0x0E2C),  # EasternSeaFissionDatabase
    (1, "DB15.BIH",      0x08944, 0x0AE6),  # GentlemanLoserDatabase
    (1, "DB16.BIH",      0x0942A, 0x0A4A),  # TozokuImportsDatabase
    (1, "DB17.BIH",      0x09E74, 0x091F),  # HosakaDatabase
    (1, "DB18.BIH",      0x0A793, 0x0D5A),  # BankGemeinschaftDatabase
    (1, "DB19.BIH",      0x0B4ED, 0x0A99),  # MusaboriDatabase
    (1, "DB20.BIH",      0x0BF86, 0x07B5),  # NASADatabase
    (1, "DB21.BIH",      0x0C73B, 0x0A85),  # BankZurichDatabase
    (1, "DB22.BIH",      0x0D1C0, 0x05F0),  # CentralJusticeDatabase
    (1, "DB23.BIH",      0x0D7B0, 0x0508),  # BellEuropaDatabase
    (1, "DB24.BIH",      0x0DCB8, 0x09B2),  # NihilistDatabase
    (1, "DB25.BIH",      0x0E66A, 0x0A29),  # INSADatabase
    (1, "DB26.BIH",      0x0F093, 0x02E1),  # SenseNetDatabase
    (1, "DB27.BIH",      0x0F374, 0x069A),  # GridpointDatabase
    (1, "DB28.BIH",      0x0FA0E, 0x03F1),  # ComSatDatabase
    (1, "DB29.BIH",      0x0FDFF, 0x0AD2),  # FreeSexUnionDatabase
    (1, "DB30.BIH",      0x108D1, 0x0C90),  # BankBerneDatabase
    (1, "DB31.BIH",      0x11561, 0x0793),  # DARPODatabase
    (1, "DB32.BIH",      0x11CF4, 0x0953),  # TuringRegistryDatabase
    (1, "DB33.BIH",      0x12647, 0x087F),  # ScreamingFistDatabase
    (1, "DB34.BIH",      0x12EC6, 0x086D),  # MaasBiolabsDatabase
    (1, "DB35.BIH",      0x13733, 0x02DB),  # KGBDatabase
    (1, "DB36.BIH",      0x13A0E, 0x0627),  # PhantomDatabase
    (1, "DB37.BIH",      0x14035, 0x0967),  # TessierAshpoolDatabase
    (1, "DB38.BIH",      0x1499C, 0x001B),  # AllardTechDatabase
    (1, "FUJI0.BIH",     0x149B7, 0x00B4),  # BAMA list
    (1, "HITACHI0.BIH",  0x14A6B, 0x0107),  # BAMA list
    (1, "HOSA0.BIH",     0x14B72, 0x00B9),  # BAMA list
    (1, "ICE.BIH",       0x14C2B, 0x0450),
    (1, "IRS0.BIH",      0x1507B, 0x0087),  # BAMA list
    (1, "IRS1.BIH",      0x15102, 0x02A1),
    (1, "JUSTICE0.BIH",  0x153A3, 0x0095),  # BAMA list
    (1, "JUSTICE1.BIH",  0x15438, 0x0099),  # BAMA list
    (1, "NEWS.BIH",      0x154D1, 0x146E),
    (1, "PAXBBS.BIH",    0x1693F,  0xC6F),
    (1, "POLICE0.BIH",   0x175AE, 0x008B),  # BAMA list
    (1, "PSYCHO0.BIH",   0x17639, 0x030D),
    (1, "PSYCHO1.BIH",   0x17946, 0x03E1),  # Fixed: was 0xE103 (Java enum typo)
    (1, "PSYCHO2.BIH",   0x17D27, 0x040C),
    (1, "PSYCHO3.BIH",   0x18133, 0x0265),
    (1, "SEA0.BIH",      0x18398, 0x008C),  # BAMA list
    (1, "SEA1.BIH",      0x18424, 0x008D),  # BAMA list
    (1, "SEA2.BIH",      0x184B1, 0x028A),
    # NEURO2.DAT - Room files (second half)
    (1, "R29.BIH",     0x32BB3,   0x45D),
    (1, "R30.BIH",     0x3427A,    0x26),
    (1, "R31.BIH",     0x34F59,    0x2D),
    (1, "R32.BIH",     0x362C1,   0x68B),
    (1, "R33.BIH",     0x37575,    0x26),
    (1, "R34.BIH",     0x37FD5,   0x498),
    (1, "R35.BIH",     0x38E5C,    0xC8),
    (1, "R36.BIH",     0x39962,   0x589),
    (1, "R37.BIH",     0x3C0E0,    0x2D),
    (1, "R38.BIH",     0x3D360,    0x2D),
    (1, "R39.BIH",     0x3E8FD,    0x2D),
    (1, "R40.BIH",     0x3FD27,   0x532),
    (1, "R41.BIH",     0x40F92,   0x284),
    (1, "R42.BIH",     0x41A5B,   0x12E),
    (1, "R44.BIH",     0x424F6,   0x918),
    (1, "R45.BIH",     0x444CF,    0xD8),
    (1, "R46.BIH",     0x460DA,   0x7F7),
    (1, "R47.BIH",     0x473B3,    0xAF),
    (1, "R49.BIH",     0x487FE,    0x2D),
    (1, "R50.BIH",     0x49CE2,   0x5E7),
    (1, "R51.BIH",     0x4CB0A,    0xBF),
    (1, "R52.BIH",     0x4D68C,   0x466),
    (1, "R53.BIH",     0x4FF5E,   0x510),
    (1, "R54.BIH",     0x5142F,    0x26),
    (1, "R55.BIH",     0x527EB,    0x26),
    (1, "R56.BIH",     0x53BC2,   0x388),
    (1, "R57.BIH",     0x5555C,   0x209),
    (1, "R58.BIH",     0x56D5C,    0xB3),
]

# TXH resources (from TxhResource.java)
RES_TXH = [
    (0, "PAXCODES.TXH", 0x3C159, 0x018A),
    (0, "FTUSER.TXH",   0x3C2E3, 0x0362),
    (0, "ROMCON0.TXH",  0x3C645, 0x014D),
    (0, "ROMCON1.TXH",  0x3C792, 0x010F),
    (0, "ROMCON2.TXH",  0x3C8A1, 0x00CD),
    (1, "ICE.TXH",      0x32762, 0x0451),
    (1, "AITALK.TXH",   0x31DCE, 0x059B),
    (1, "ENDGAME.TXH",  0x32369, 0x03F9),
]


# ====================================================================
# Database number -> name mapping (from Database.java subclasses)
# ====================================================================

DATABASE_NAMES = OrderedDict([
    (0,  "Regular Fellows"),
    (1,  "Consumer Review"),
    (2,  "Asano's"),
    (3,  "World Chess"),
    (4,  "Cheap Hotel"),
    (5,  "Psychologist (Chrome)"),
    (6,  "Panther Moderns"),
    (7,  "IRS"),
    (8,  "Fuji Electric"),
    (9,  "Chiba City Police"),
    (10, "Hitachi Biotech"),
    (11, "Copenhagen University"),
    (12, "Software Enforcement"),
    (13, "Free Matrix (Sapphire)"),
    (14, "Eastern Sea Fission"),
    (15, "Gentleman Loser"),
    (16, "Tozoku Imports"),
    (17, "Hosaka"),
    (18, "Bank Gemeinschaft"),
    (19, "Musabori (Greystoke)"),
    (20, "NASA (HAL)"),
    (21, "Bank Zurich"),
    (22, "Central Justice"),
    (23, "Bell Europa"),
    (24, "Nihilist"),
    (25, "INSA"),
    (26, "Sense/Net"),
    (27, "Gridpoint"),
    (28, "ComSat"),
    (29, "Free Sex Union (Xaviera)"),
    (30, "Bank Berne (Gold)"),
    (31, "DARPO"),
    (32, "Turing Registry"),
    (33, "Screaming Fist"),
    (34, "Maas Biolabs (Sangfroid)"),
    (35, "KGB (Lucifer)"),
    (36, "Phantom"),
    (37, "Tessier-Ashpool (Wintermute)"),
    (38, "Allard Tech (Neuromancer)"),
])

# Room names
ROOM_NAMES = {
    1:  "Chatsubo", 2: "Street Chatsubo", 3: "Cyber Justice",
    4:  "Body Shop", 5: "Street Donut World", 6: "Donut World",
    7:  "Cheap Hotel", 8: "Gentleman Loser", 9: "Maas Biolabs",
    10: "JAL Shuttle", 11: "Zion", 12: "Larry's Software",
    13: "Street Maas Biolabs 1", 14: "Street Maas Biolabs 2",
    15: "Street Maas Biolabs 3", 16: "Street Maas Biolabs 4",
    17: "Street Maas", 18: "Street Maas 2", 19: "Spaceport",
    20: "Marcus Garvey", 21: "Street Spaceport", 22: "Villa Straylight",
    23: "Panther Moderns", 24: "Massage Parlor", 25: "Shin's Pawn",
    26: "Street Light Pole", 27: "Julius Dean", 28: "JAL Shuttle (Return)",
    29: "Freeside Spacedock", 30: "Street Freeside", 31: "Street Freeside 2",
    32: "Metro Holographix", 33: "Street Metro", 34: "Bank of Berne Lobby",
    35: "Bank Manager's Office", 36: "House of Pong",
    37: "Street Pong 1", 38: "Street Pong 2", 39: "Street Pong 3",
    40: "Crazy Edo's", 41: "Bank Gemeinschaft", 42: "Street Bank",
    44: "Asano's", 45: "Street Security Robot", 46: "Matrix Restaurant",
    47: "Street Matrix", 49: "Street Hosaka", 50: "Cyberspace Beach",
    51: "Fuji Electric", 52: "Security Gate", 53: "Hitachi Biotech",
    54: "High Tech Area 1", 55: "High Tech Area 2", 56: "SenseNet HQ",
    57: "Hosaka HQ", 58: "Musabori Headquarters",
}

# PAX NEWS headers (from data.c g_seg004)
PAX_NEWS_HEADERS = [
    {"date": "11/16/58", "subject": "BAR FOOD DECLARED FATAL"},
    {"date": "11/16/58", "subject": "COWBOY DISAPPEARS"},
    {"date": "11/16/58", "subject": "News In Brief"},
    {"date": "11/17/58", "subject": "NASA AND FUJI DO BUSINESS", "condition": "date_day > 1"},
    {"date": "11/17/58", "subject": "News In Brief", "condition": "date_day > 1"},
    {"date": "11/18/58", "subject": "JUSTICE DEFENDS DEFENDERS", "condition": "date_day != 1"},
    {"date": "11/18/58", "subject": "News In Brief", "condition": "date_day != 1"},
    {"date": "11/18/58", "subject": "FRIED COWBOY FOUND", "condition": "date_day != 1"},
    {"date": "11/19/58", "subject": "DR. TIMOTHY LEARY AT 138", "condition": "date_day != 1"},
    {"date": "11/19/58", "subject": "DISMEMBERED HAND FOUND", "condition": "date_day != 1"},
    {"date": "11/19/58", "subject": "News In Brief", "condition": "date_day != 1"},
    {"date": "11/00/58", "subject": "PERVERT NETTED IN SWEEP", "condition": "flag_4C1F", "dynamic_date": True},
    {"date": "11/00/58", "subject": "CRIMINAL HITS CHIBA CITY", "condition": "flag_4C59", "dynamic_date": True},
    {"date": "11/00/58", "subject": "CHIBA CITY HITS CRIMINAL", "condition": "flag_4C5A", "dynamic_date": True},
    {"date": "11/00/58", "subject": "MAAS BIOLABS BURGLARIZED", "condition": "flag_4C23", "dynamic_date": True},
    {"date": "11/00/58", "subject": "INDUSTRIAL SPY NABBED", "condition": "flag_4C27", "dynamic_date": True},
    {"date": "11/00/58", "subject": "JUSTICE BLINDED", "condition": "flag_4C29", "dynamic_date": True},
    {"date": "11/00/58", "subject": "SENSE/NET RAIDED AGAIN", "condition": "flag_4C25", "dynamic_date": True},
    {"date": "11/00/58", "subject": "VAGRANT PAYS HOTEL BILL", "condition": "flag_4C2B", "dynamic_date": True},
    {"date": "11/00/58", "subject": "BANK LOSES MONEY", "condition": "flag_4C2D", "dynamic_date": True},
]

PAX_BBS_HEADERS = [
    {"date": "11/14/58", "to": "All",      "from": "SysOp"},
    {"date": "11/14/58", "to": "PLAYER",   "from": "Matt Shaw"},
    {"date": "11/14/58", "to": "PLAYER",   "from": "FFargo"},
    {"date": "11/14/58", "to": "PLAYER",   "from": "Shin"},
    {"date": "11/14/58", "to": "PLAYER",   "from": "Crazy Edo"},
    {"date": "11/15/58", "to": "PLAYER",   "from": "Matt Shaw"},
    {"date": "11/15/58", "to": "PLAYER",   "from": "Bosch"},
    {"date": "11/15/58", "to": "PLAYER",   "from": "Emp. Norton"},
    {"date": "11/16/58", "to": "Ratz",     "from": "Red Snake"},
    {"date": "11/16/58", "to": "All",      "from": "Interplay"},
    {"date": "11/16/58", "to": "All",      "from": "Armitage"},
    {"date": "11/16/58", "to": "All",      "from": "Hitachi"},
    {"date": "11/17/58", "to": "PLAYER",   "from": "Emp. Norton", "condition": "date_day > 1"},
    {"date": "11/17/58", "to": "All",      "from": "CFM",         "condition": "date_day > 1"},
    {"date": "11/17/58", "to": "All",      "from": "IRS",         "condition": "date_day > 1"},
    {"date": "11/18/58", "to": "Larry",    "from": "Modern Bob",  "condition": "date_day != 1"},
    {"date": "11/18/58", "to": "Crazy Edo","from": "Wakizashi",   "condition": "date_day != 1"},
    {"date": "11/19/58", "to": "Wakizashi","from": "Crazy Edo",   "condition": "date_day != 1"},
    {"date": "11/16/58", "to": "PLAYER",   "from": "Bosch",       "condition": "flag_4BF1"},
    {"date": "11/16/58", "to": "PLAYER",   "from": "Armitage",    "condition": "flag_4C21"},
    {"date": "11/16/58", "to": "All",      "from": "Sense/Net",   "condition": "flag_4C25"},
]


# ====================================================================
# Huffman decompression
# ====================================================================

class HuffmanBitReader:
    def __init__(self, data):
        self.data = data
        self.pos = 0
        self.bit_mask = 0

    def getbits(self, n):
        result = 0
        for _ in range(n):
            if self.bit_mask == 0:
                if self.pos >= len(self.data):
                    raise IndexError("Huffman: read past end of data at pos=%d/%d" % (self.pos, len(self.data)))
                self.bit_mask = 0x80
                self.byte = self.data[self.pos]
                self.pos += 1
            result = (result << 1) | (1 if (self.byte & self.bit_mask) else 0)
            self.bit_mask >>= 1
        return result

    def read_le32(self):
        if self.pos + 4 > len(self.data):
            raise IndexError("Huffman: length prefix past end of data")
        val = struct.unpack('<I', self.data[self.pos:self.pos+4])[0]
        self.pos += 4
        return val


class HuffmanNode:
    __slots__ = ('value', 'left', 'right')
    def __init__(self, value=None, left=None, right=None):
        self.value = value
        self.left = left
        self.right = right
    def is_leaf(self):
        return self.left is None


def _build_tree(reader):
    if reader.getbits(1):
        value = reader.getbits(8)
        return HuffmanNode(value=value)
    right = _build_tree(reader)
    left = _build_tree(reader)
    return HuffmanNode(left=left, right=right)


def huffman_decompress(data):
    reader = HuffmanBitReader(data)
    length = reader.read_le32()
    root = _build_tree(reader)
    result = bytearray(length)
    node = root
    i = 0
    while i < length:
        node = node.left if reader.getbits(1) else node.right
        if node.is_leaf():
            result[i] = node.value
            i += 1
            node = root
    return bytes(result)


# ====================================================================
# BIH parsing
# ====================================================================

def parse_bih_header(data):
    if len(data) < 0x20:
        return None
    return {
        'text_offset': struct.unpack_from('<H', data, 0x06)[0],
        'bytecode_array_offt': [struct.unpack_from('<H', data, 0x08+i*2)[0] for i in range(3)],
        'init_obj_code_offt': [struct.unpack_from('<H', data, 0x0E+i*2)[0] for i in range(3)],
    }


def extract_strings(data, offset=0):
    """Extract null-terminated strings from data[offset:], skipping empty ones."""
    strings = []
    if offset >= len(data):
        return strings
    pos = offset
    while pos < len(data):
        end = data.find(0, pos)
        if end == -1:
            end = len(data)
        s = data[pos:end].decode('ascii', errors='replace')
        if s.strip():
            strings.append(s)
        pos = end + 1
        if pos >= len(data):
            break
    return strings


def extract_bama_list(data):
    """Extract BAMA list entries (32-byte chunks: name[18]+null, number[9]+null, flags[2]+null)."""
    entries = []
    i = 0
    while i + 31 < len(data) and data[i] != 0:
        name = data[i:i+18].decode('ascii', errors='replace').rstrip('\x00').strip()
        bama = data[i+19:i+28].decode('ascii', errors='replace').rstrip('\x00').strip()
        entries.append({"name": name, "bama": bama})
        i += 32
    return entries


# ====================================================================
# Main extraction
# ====================================================================

def load_dat_files(base_dir):
    dat_files = {}
    for name in ("NEURO1.DAT", "NEURO2.DAT"):
        path = os.path.join(base_dir, name)
        if not os.path.exists(path):
            for candidate in ["sdlmancer/src/reuromancer", "sdlmancer/src/reuromancer/NEURO1"]:
                alt = os.path.join(base_dir, candidate, name)
                if os.path.exists(alt):
                    path = alt
                    break
        if not os.path.exists(path):
            print(f"ERROR: {name} not found", file=sys.stderr)
            sys.exit(1)
        with open(path, 'rb') as f:
            dat_files[name] = f.read()
    return dat_files


def load_resource(dat_files, file_idx, offset, size):
    dat_name = "NEURO1.DAT" if file_idx == 0 else "NEURO2.DAT"
    return dat_files[dat_name][offset:offset+size]


def try_decompress(compressed):
    """Try Huffman decompression; return (data, error)."""
    try:
        return huffman_decompress(compressed), None
    except Exception as e:
        return None, str(e)


def extract_all_text(base_dir="."):
    dat_files = load_dat_files(base_dir)
    output = OrderedDict()

    # ---- 1. ROOMS (R1-R58 BIH) ----
    output["rooms"] = OrderedDict()
    for fidx, name, offset, size in RES_BIH:
        m = __import__('re').match(r'^R(\d+)\.BIH$', name)
        if not m:
            continue
        room_num = int(m.group(1))
        if room_num < 1 or room_num > 58:
            continue

        compressed = load_resource(dat_files, fidx, offset, size)
        decomp, err = try_decompress(compressed)
        if err:
            output["rooms"][f"R{room_num}"] = {
                "name": ROOM_NAMES.get(room_num, f"Room {room_num}"),
                "error": err,
            }
            continue

        hdr = parse_bih_header(decomp)
        text_offset = hdr['text_offset'] if hdr else 0
        strings = extract_strings(decomp, text_offset)

        result = {
            "name": ROOM_NAMES.get(room_num, f"Room {room_num}"),
            "string_count": len(strings),
            "strings": strings,
        }
        if strings:
            result["long_description"] = strings[0]
        if len(strings) > 1:
            result["short_description"] = strings[1]
        if len(strings) > 2:
            result["dialog_text"] = strings[2:]
        output["rooms"][f"R{room_num}"] = result

    # ---- 2. DATABASES (DB0-DB38 BIH) ----
    output["databases"] = OrderedDict()
    for fidx, name, offset, size in RES_BIH:
        m = __import__('re').match(r'^DB(\d+)\.BIH$', name)
        if not m:
            continue
        db_num = int(m.group(1))
        db_name = DATABASE_NAMES.get(db_num, f"Database {db_num}")

        compressed = load_resource(dat_files, fidx, offset, size)
        decomp, err = try_decompress(compressed)
        if err:
            output["databases"][f"DB{db_num}"] = {
                "name": db_name,
                "error": err,
            }
            continue

        # DB files have a different header format (from BIHThing.java):
        # byte 6+7 = text offset (LE), byte 10+11 = password offset, byte 12+13 = menu control offset
        text_offset = struct.unpack_from('<H', decomp, 0x06)[0]
        pw_offset = struct.unpack_from('<H', decomp, 0x0A)[0]
        menu_offset = struct.unpack_from('<H', decomp, 0x0C)[0]

        strings = extract_strings(decomp, text_offset)
        passwords = []
        if pw_offset and menu_offset and pw_offset < menu_offset:
            passwords = extract_strings(decomp, pw_offset)
            # But passwords may overlap with strings; the Java code extracts them separately
            # For safety, extract from pw_offset up to menu_offset
            pw_data = decomp[pw_offset:menu_offset]
            passwords = [s for s in extract_strings(pw_data, 0) if s.strip()]

        result = {
            "name": db_name,
            "db_number": db_num,
            "string_count": len(strings),
            "text": strings,
        }
        if passwords:
            result["passwords"] = passwords
        output["databases"][f"DB{db_num}"] = result

    # ---- 3. NEWS ----
    output["news"] = OrderedDict()
    for fidx, name, offset, size in RES_BIH:
        if name != "NEWS.BIH":
            continue
        compressed = load_resource(dat_files, fidx, offset, size)
        decomp, err = try_decompress(compressed)
        if err:
            output["news"] = {"error": err}
            continue
        strings = extract_strings(decomp, 0)
        articles = []
        for i, s in enumerate(strings):
            hdr = PAX_NEWS_HEADERS[i] if i < len(PAX_NEWS_HEADERS) else {}
            article = {"index": i, "body": s}
            if hdr:
                article["date"] = hdr.get("date", "")
                article["subject"] = hdr.get("subject", "")
                if "condition" in hdr:
                    article["condition"] = hdr["condition"]
                if "dynamic_date" in hdr:
                    article["dynamic_date"] = True
            articles.append(article)
        output["news"]["articles"] = articles

    # ---- 4. BBS ----
    output["bbs"] = OrderedDict()
    for fidx, name, offset, size in RES_BIH:
        if name != "PAXBBS.BIH":
            continue
        compressed = load_resource(dat_files, fidx, offset, size)
        decomp, err = try_decompress(compressed)
        if err:
            output["bbs"] = {"error": err}
            continue
        strings = extract_strings(decomp, 0)
        messages = []
        for i, s in enumerate(strings):
            hdr = PAX_BBS_HEADERS[i] if i < len(PAX_BBS_HEADERS) else {}
            msg = {"index": i, "body": s}
            if hdr:
                msg["date"] = hdr.get("date", "")
                msg["to"] = hdr.get("to", "")
                msg["from"] = hdr.get("from", "")
                if "condition" in hdr:
                    msg["condition"] = hdr["condition"]
            messages.append(msg)
        output["bbs"]["messages"] = messages

    # ---- 5. TXH FILES ----
    output["txh"] = OrderedDict()
    for fidx, name, offset, size in RES_TXH:
        compressed = load_resource(dat_files, fidx, offset, size)
        decomp, err = try_decompress(compressed)
        key = name.replace(".TXH", "").lower()
        if err:
            output["txh"][key] = {"source": name, "error": err}
            continue
        strings = extract_strings(decomp, 0)
        output["txh"][key] = {
            "source": name,
            "string_count": len(strings),
            "strings": strings,
            "full_text": "\n".join(strings),
        }

    # ---- 6. SPECIAL BIH FILES ----
    output["special_bih"] = OrderedDict()
    special_names = {"COPEN0", "COPEN1", "PSYCHO0", "PSYCHO1", "PSYCHO2", "PSYCHO3",
                     "ICE", "IRS1", "SEA2", "JUSTICE0", "JUSTICE1"}
    bama_names = {"FUJI0", "HITACHI0", "HOSA0", "IRS0", "POLICE0", "SEA0", "SEA1"}

    for fidx, name, offset, size in RES_BIH:
        base = name.replace(".BIH", "")
        if base in special_names:
            compressed = load_resource(dat_files, fidx, offset, size)
            decomp, err = try_decompress(compressed)
            if err:
                output["special_bih"][base] = {"error": err}
                continue
            # Plain text files (COPEN, PSYCHO, IRS1, SEA2, JUSTICE)
            strings = extract_strings(decomp, 0)
            output["special_bih"][base] = {
                "type": "text",
                "string_count": len(strings),
                "strings": strings,
            }
        elif base in bama_names:
            compressed = load_resource(dat_files, fidx, offset, size)
            decomp, err = try_decompress(compressed)
            if err:
                output["special_bih"][base] = {"error": err}
                continue
            entries = extract_bama_list(decomp)
            output["special_bih"][base] = {
                "type": "bama_list",
                "entry_count": len(entries),
                "entries": entries,
            }
        elif name == "ICE.BIH":
            compressed = load_resource(dat_files, fidx, offset, size)
            decomp, err = try_decompress(compressed)
            if err:
                output["special_bih"]["ICE_BIH"] = {"error": err}
                continue
            strings = extract_strings(decomp, 0)
            output["special_bih"]["ICE_BIH"] = {
                "type": "text",
                "string_count": len(strings),
                "strings": strings,
            }

    return output


def main():
    base_dir = sys.argv[1] if len(sys.argv) > 1 else "."

    if not os.path.exists(os.path.join(base_dir, "NEURO1.DAT")):
        for candidate in ["sdlmancer/src/reuromancer", "sdlmancer/src/reuromancer/NEURO1"]:
            if os.path.exists(os.path.join(base_dir, candidate, "NEURO1.DAT")):
                base_dir = os.path.join(base_dir, candidate)
                break

    print(f"Extracting text from: {base_dir}", file=sys.stderr)
    result = extract_all_text(base_dir)

    print(json.dumps(result, indent=2, ensure_ascii=False))

    # Summary to stderr
    print("\n=== EXTRACTION SUMMARY ===", file=sys.stderr)
    rooms = result.get("rooms", {})
    ok = sum(1 for r in rooms.values() if "strings" in r)
    fail = sum(1 for r in rooms.values() if "error" in r)
    print(f"Rooms: {ok} OK, {fail} failed, {len(rooms)} total", file=sys.stderr)

    dbs = result.get("databases", {})
    d_ok = sum(1 for d in dbs.values() if "text" in d)
    d_fail = sum(1 for d in dbs.values() if "error" in d)
    print(f"Databases: {d_ok} OK, {d_fail} failed, {len(dbs)} total", file=sys.stderr)

    total_strings = 0
    total_chars = 0
    for section in ["rooms", "databases", "news", "bbs", "txh", "special_bih"]:
        data = result.get(section, {})
        if section == "rooms":
            for v in data.values():
                if "strings" in v:
                    total_strings += len(v["strings"])
                    total_chars += sum(len(s) for s in v["strings"])
        elif section == "databases":
            for v in data.values():
                if "text" in v:
                    total_strings += len(v["text"])
                    total_chars += sum(len(s) for s in v["text"])
        elif section == "news":
            for a in data.get("articles", []):
                total_strings += 1
                total_chars += len(a.get("body", ""))
        elif section == "bbs":
            for m in data.get("messages", []):
                total_strings += 1
                total_chars += len(m.get("body", ""))
        elif section == "txh":
            for v in data.values():
                if "strings" in v:
                    total_strings += len(v["strings"])
                    total_chars += sum(len(s) for s in v["strings"])
        elif section == "special_bih":
            for v in data.values():
                if "strings" in v:
                    total_strings += len(v["strings"])
                    total_chars += sum(len(s) for s in v["strings"])

    print(f"\nTotal: {total_strings} strings, {total_chars} characters", file=sys.stderr)


if __name__ == "__main__":
    main()
