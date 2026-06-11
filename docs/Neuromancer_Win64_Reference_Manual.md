# Neuromancer Win64 (Reuromancer) -- Reference Manual

> Supplement to the Java reference manual. Documents the Win64 C port by
> Henadzi Matuts. Use together with `Neuromancer_Java_Reference_Manual.md`.
>
> This port takes a fundamentally different approach: it **emulates the original
> DOS game's CPU (8086)** and **BIH bytecode VM**, rather than reimplementing
> game logic from scratch like the Java port does.

---

## TABLE OF CONTENTS

1. [Architecture Overview](#1-architecture-overview)
2. [Fundamental Design Difference](#2-fundamental-design-difference)
3. [Scene System](#3-scene-system)
4. [Main Menu Scene](#4-main-menu-scene)
5. [Real World Scene](#5-real-world-scene)
6. [8086 CPU Emulator](#6-8086-cpu-emulator)
7. [Neuro VM (BIH Bytecode)](#7-neuro-vm-bih-bytecode)
8. [Global Data Structures](#8-global-data-structures)
9. [Dialog State](#9-dialog-state)
10. [PAX System](#10-pax-system)
11. [Inventory System](#11-inventory-system)
12. [Body Parts Shop](#12-body-parts-shop)
13. [Skills / ROM / Disk States](#13-skills--rom--disk-states)
14. [Character Control](#14-character-control)
15. [Drawing System](#15-drawing-system)
16. [Window / Menu System](#16-window--menu-system)
17. [Resource Manager](#17-resource-manager)
18. [Save/Load System](#18-saveload-system)
19. [Address Translation](#19-address-translation)
20. [LibNeuroRoutines](#20-libneuroroutines)
21. [Java <-> Win64 Mapping Guide](#21-java---win64-mapping-guide)
22. [Index: File -> Function Map](#22-index-file---function-map)

---

## 1. ARCHITECTURE OVERVIEW

The Win64 port consists of three modules:

| Module | Directory | Purpose |
|--------|-----------|---------|
| `NeuromancerWin64/` | Main game executable | Scene management, state handlers, input, CPU emulation |
| `LibNeuroRoutines/` | Static/shared library | Resource decompression, drawing primitives, sound, animations |
| `ResourceBrowser/` | MFC GUI tool | DAT resource browser and extraction utility |

**Dependencies:** SFML (Simple and Fast Multimedia Library) for windowing, input, and audio.

---

## 2. FUNDAMENTAL DESIGN DIFFERENCE

### Java Port (Javamancer)
- **Reimplementation:** All game logic is rewritten in Java with explicit classes for rooms, AI, databases, items, etc.
- **Data-driven but interpreted:** Dialog chains, room extras, and warez effects are implemented as Java methods.
- **Modern OOP:** OOP throughout, reflection for factory patterns.
- **JavaFX UI:** Declarative UI with scene graphs.

### Win64 Port (Reuromancer)
- **Emulation:** The original DOS game logic runs inside an **8086 CPU emulator**.
- **BIH Bytecode:** Each room has a `.BIH` file containing a custom bytecode program (the "Neuro VM") that handles room logic, dialog flow, NPC interactions.
- **Callback mechanism:** The C host calls into the emulated game via `neuro_cb()` for specific operations (body shop, inventory queries, NPC reply lookup, etc.).
- **Segment memory:** Original 16-bit segment:offset addressing translated to 64-bit linear addresses via `address_translator.c`.
- **State machines:** UI states (inventory, PAX, dialog, body shop) are explicit C state machines.

**Key implication for C port:** You have two options:
1. Follow the Win64 approach: port the 8086 emulator + Neuro VM + callback system.
2. Follow the Java approach: reimplement all game logic explicitly.

---

## 3. SCENE SYSTEM

**File:** `scene_control.h` / `scene_control.c`

The game uses a scene-based architecture:

```c
typedef enum neuro_scene_id_t {
    NSID_NONE = -1,
    NSID_MAIN_MENU = 0,
    NSID_REAL_WORLD = 1,
    NSID_NOT_IMPLEMENTED = 255,
} neuro_scene_id_t;

typedef struct neuro_scene_t {
    neuro_scene_id_t id;
    void(*init)(void);
    void(*deinit)(void);
    void(*handle_input)(sfEvent *event);
    neuro_scene_id_t(*update)(void);
} neuro_scene_t;

extern neuro_scene_t g_scene;

void scene_control_setup_scene(neuro_scene_id_t id);
```

Scene transition: `deinit()` current scene, then `init()` new scene.

**Game loop** (in `main.c`):
```
while (!g_exit_game) {
    sfRenderWindow_pollEvents(window, &event);
    g_scene.handle_input(&event);
    next_scene = g_scene.update();
    if (next_scene != g_scene.id)
        scene_control_setup_scene(next_scene);
    drawing_control_draw();
    sfRenderWindow_display(window);
}
```

---

## 4. MAIN MENU SCENE

**File:** `scene_main_menu.c`

### State Machine
```
MMS_INITIAL -> New game pressed -> MMS_NEW (name input)
MMS_INITIAL -> Load pressed -> MMS_LOAD (slot selection)
MMS_NEW -> Name entered -> MMS_TO_LEVEL_SCENE (fade transition)
MMS_LOAD -> Slot loaded -> MMS_TO_LEVEL_SCENE
MMS_TO_LEVEL_SCENE -> Fade complete -> NSID_REAL_WORLD
```

### Key Functions
```
init(): Load TITLE.IMH, create New/Load menu
handle_input(): Route to neuro_menu_handle_input()
update(): Process fade animation, transition to Real World
deinit(): Remove background sprite
```

---

## 5. REAL WORLD SCENE

**File:** `scene_real_world.c`, `scene_real_world.h`

### State Machine
```
RWS_FADE_IN -> Fade complete -> RWS_TEXT_OUTPUT
RWS_TEXT_OUTPUT -> Text scroll done -> RWS_WAIT_FOR_INPUT -> RWS_NORMAL
RWS_NORMAL -> Player action -> RWS_INVENTORY | RWS_PAX | RWS_DIALOG | RWS_SKILLS | RWS_ROM | RWS_DISK_OPTIONS | RWS_BODY_PARTS_SHOP
RWS_*_STATE -> Exit -> RWS_NORMAL
RWS_RELOAD_LEVEL -> Level transition fade -> RWS_FADE_IN
```

### Level Transition
```
level_transition_prepare_anim(exit):
    Certain room groups skip animation (rooms 12-17, 36-38, 53-54)
    Setup screen fade (FADE_OUT for exit, FADE_IN for entry)
```

### Level Init
```
init():
    Load NEURO.IMH (background)
    Load R<N>.BIH (room data)
    Load R<N>.PIC (room backdrop)
    Load R<N>.ANH (animation data)
    sub_105F6(PREPARE_BIH, level)     // Setup BIH header
    Setup VM states from vm_state table
    roompos_init()                     // Calculate player start position
    sub_105F6(INIT_LEVEL)              // Run level init code in 8086 emulator
    setup_ui_buttons()                 // Setup button bar (inventory, PAX, dialog, etc.)
```

### UI Buttons
```
rw_ui_handle_button_press():
    code 0x00 -> Inventory
    code 0x01 -> PAX
    code 0x02 -> Dialog
    code 0x03 -> Skills
    code 0x04 -> ROM
    code 0x05 -> Disk Options
    code 0x0A -> Date display
    code 0x0B -> Time display
    code 0x0C -> Cash display
    code 0x0D -> Constitution display
```

### Player Movement & Exit Detection
```
update_normal():
    character_control_update()         // Move player character
    sub_105F6(UPDATE_LEVEL)            // Run level update code
    sub_105F6(NEURO_VM_CYCLE)          // Run VM bytecode
    
    if character direction != NULL:
        g_exit_point = direction
        level = roompos_hit_exit_zone()
        if level != -1:
            g_4bae.level_n = level
            g_state = RWS_RELOAD_LEVEL
```

### Time System
```
ui_panel_update():
    Every 1000ms:
        time_m++
        if time_m == 60: time_m = 0, time_h++
        if time_h == 24: time_h = 0, date_day++
```

---

## 6. 8086 CPU EMULATOR

**File:** `neuro86.c`, `neuro86.h`, `opcodes.h`

### CPU Structure
```c
typedef struct cpu_s {
    uint8_t regs[REG_COUNT * 2];     // AX, CX, DX, BX, SP, BP, SI, DI (each 16-bit)
    uint16_t flags;                   // CF, PF, AF, ZF, SF, TF, IF, DF, OF
    uint16_t ip;                      // Instruction pointer (within 64KB segment)
    pfn_far_call_cb callback;         // Far call callback (traps to host)
    uint8_t state;                    // CPU_STOPPED, CPU_RUNNING, CPU_HALTED
} cpu_t;
```

### Supported Opcodes
Partial 8086 instruction set. Implemented:
- ALU: ADD, OR, ADC, SBB, AND, SUB, XOR, CMP, TEST (8/16-bit)
- INC/DEC registers (0x40-0x47, 0x48-0x4F)
- PUSH/POP (0x50-0x57, 0x58-0x5F)
- Conditional jumps (0x70-0x7F): JO, JNO, JB, JNB, JZ, JNZ, JBE, JA, JS, JNS, JP, JNP, JL, JNL, JLE, JNLE
- MOV: 0x88-0x8B, 0xC6-0xC7, 0xB0-0xB7, 0xB8-0xBF
- CALL: 0xE8 (near relative)
- RET: 0xC3
- JMP: 0xEB (near relative)
- LOOP: 0xE2
- INC/DEC/Memory: 0xFE, 0xFF
- XLAT: 0xD7
- RETF: 0xCB (halt)

### Far Call Callback
Opcode 0xFF with reg=3 triggers a far call, which invokes:
```c
cpu->callback(get_reg_u16(cpu, REG_SP))
```
This returns `CPU_STOPPED` to pause or `CPU_RUNNING` to continue.

The `neuro_cb()` function handles the callback:

```c
uint8_t neuro_cb(uint16_t sp) {
    cmd = read from stack;
    switch (cmd) {
        case 0:  reset_vm();
        case 2:  has_item(item_code);         // Sets g_4bae.x4c82
        case 3:  remove_item(item_code);
        case 5:  npc_reply(string, num);      // Sets g_4bae.x4c82
        case 8:  g_body_shop_op = 1; g_state = RWS_BODY_PARTS_SHOP; return CPU_STOPPED;
        case 9:  g_body_shop_discount = param; g_body_shop_op = 0; g_state = RWS_BODY_PARTS_SHOP; return CPU_STOPPED;
        case 31: bg_animation_control_prepare();
    }
    return CPU_RUNNING;
}
```

---

## 7. NEURO VM (BIH BYTECODE)

**File:** `scene_real_world.c` (function `neuro_vm()` and `sub_105F6()`)

### VM States
```c
typedef struct neuro_vm_state_t {
    uint8_t level;             // Level this VM state belongs to
    uint8_t flag;              // Thread number (0-3) + flags
    uint8_t mark;              // 0xFF = not initialized
    uint16_t vm_next_op_addr;  // Next bytecode instruction address
    uint8_t var_1;             // Position variable (used for dialog bubbles)
    uint8_t var_2;             // Position variable
} neuro_vm_state_t;

// 35 VM states total (g_3f85.vm_state[35])
// 4 threads per level (g_a8e0.a8e0[4])
```

### Bytecode Opcodes
```
0x00: Get program address (set vm_next_op_addr from bytecode array)
0x01: Dialog NPC reply (show text in bubble at var_1, var_2)
0x02: Text output (scrolling text in neuro window)
0x03: Load program (set vm_next_op_addr to specific program index)
0x04: GOTO (relative offset)
0x05-0x08: Conditional jump (JE, JNE, JL, JGE comparing g_4bae.x4bae[index] to value)
0x0E: Set memory (16-bit write to g_4bae.x4bae[index])
0x0F: Set memory (same as 0x0E)
0x10: Load level (transition to new level)
0x11: Update hold (g_update_hold++ -- pause VM execution)
0x12: Update release (g_update_hold = 0)
0x13: Set dialog control (set first_reply, total_replies for level)
0x15: Add memory (16-bit add to g_4bae.x4bae[index])
0x16: EXEC (run 8086 code at offset in BIH)
0x17: Dialog (enter dialog state RWS_DIALOG)
0x18: Dialog reply (show text from string_num table)
```

### VM Execution Flow
```
neuro_vm(state):
    for thread = 3 downto 0:
        n = g_a8e0.a8e0[thread]
        if n == 0xFFFF: continue
        
        opcode = read byte at vm_next_op_addr
        execute opcode
        if opcode caused state change (0x01, 0x02, 0x10, 0x17):
            return (pause VM, handle UI)
```

### BIH File Structure
```c
typedef struct bih_hdr_t {
    uint16_t cb_offt;              // 0xA8E8 - Callback offset
    uint16_t cb_segt;              // 0xA8EA - Callback segment
    uint16_t ctrl_struct_addr;     // 0xA8EC - Points to g_4bae
    uint16_t text_offset;          // 0xA8EE - Start of text strings
    uint16_t bytecode_array_offt[3]; // 0xA8F0 - Bytecode program arrays
    uint16_t init_obj_code_offt[3]; // 0xA8F6 - Init/update/deinit code offsets
} bih_hdr_t;
```

The BIH file contains:
1. Header (0xA8E8+)
2. Text strings (null-terminated, accessed via text_offset)
3. 8086 machine code (executed via CPU emulator)
4. Bytecode programs (interpreted by Neuro VM)

---

## 8. GLOBAL DATA STRUCTURES

### g_4bae (0x4BAE) - Player State
The most important global structure. Contains all mutable game state:

```c
typedef struct x4bae_t {
    uint8_t x4bae[16];                    // Generic flags/state (0x4BAE-0x4BBB)
    uint8_t active_dialog_reply;          // 0x4BBE - Current dialog reply
    uint16_t active_item;                 // 0x4BC0 - Currently selected item
    uint32_t cash_withdrawal;             // 0x4BC2 - Amount to withdraw
    uint16_t time_m;                      // 0x4BC6 - Game minutes
    uint8_t time_h;                       // 0x4BC8 - Game hour
    uint8_t date_day;                     // 0x4BC9 - Day counter
    uint8_t active_skill;                 // 0x4BF4 - Active skill index
    uint8_t active_skill_level;           // 0x4BF5 - Skill level
    uint8_t gas_mask_is_on;               // Gas mask state
    uint16_t msg_to_armitage_sent;        // Flag: message to Armitage sent
    uint32_t cash;                        // 0x4C78 - Credits on chip
    uint8_t sold_body_parts_bitstring[3]; // 0x4C84 - 24 body parts
    uint32_t bank_account;                // 0x4C89 - Bank balance
    char name[13];                        // 0x4C92 - Player name
    uint16_t constitution;                // 0x4C9F - Health
    uint16_t level_n;                     // 0x4CA1 - Current room number
    uint16_t roompos_x;                   // 0x4CA3 - Player X position
    uint16_t roompos_y;                   // 0x4CA5 - Player Y position
    transaction_record_t bank_transaction_record[4]; // 0x4CA9
    uint16_t ui_type;                     // 0x4CC1
    uint16_t frame_sc_index;              // 0x4CCF - Animation frame counter
} x4bae_t;
```

### g_3f85 (0x3F85) - Persistent State
```c
typedef struct x3f85_t {
    neuro_vm_state_t vm_state[35];    // VM states per level
    uint8_t vm_state_end;             // 0x407A - Sentinel
    level_info_t level_info[58];      // 0x407B - Per-level dialog info
    neuro_inventory_t inventory;      // 0x41D7 - Items and software
    uint8_t skills[16];               // 0x42D7 - Installed skills
    uint8_t x42e7[386];               // 0x42E7 - Various game flags
} x3f85_t;
```

### g_seg004 - PAX Data
```c
typedef struct seg004_layout_t {
    pax_news_hdr_t pax_news[21];       // News articles with conditions
    pax_board_msg_hdr_t pax_board_msg[22]; // BBS messages with conditions
} seg004_layout_t;
```

### g_seg016 - Room Positions & Animations
```c
typedef struct seg016_layout_t {
    uint8_t roompos[1160];    // Player/NPC positions per room
    uint8_t anh[22808];       // ANH animation data
} seg016_layout_t;
```

---

## 9. DIALOG STATE

**File:** `rw_state_dialog.c`

### State Machine
```
DS_OPEN_DIALOG -> (frame delay) -> DS_CHOOSE_REPLY_WFI
DS_CHOOSE_REPLY_WFI -> Left click/Space -> DS_NEXT_REPLY
DS_CHOOSE_REPLY_WFI -> Right click/Enter -> DS_ACCEPT_REPLY
DS_NEXT_REPLY -> (frame delay) -> DS_CHOOSE_REPLY_WFI (cycling replies)
DS_ACCEPT_REPLY -> (frame delay) -> DS_ACCEPT_REPLY_WFI | DS_ACCEPT_REPLY_TEXT_INPUT
DS_ACCEPT_REPLY_TEXT_INPUT -> Enter -> DS_CLOSE_DIALOG (user typed word)
DS_ACCEPT_REPLY_WFI -> Click/Key -> DS_CLOSE_DIALOG
DS_CLOSE_DIALOG -> (frame delay) -> back to RWS_NORMAL
```

### Dialog Reply System
```c
// Dialog info stored per level:
typedef struct level_info_t {
    uint8_t first_dialog_reply;       // First reply index in BIH text
    uint8_t total_dialog_replies;     // Number of player replies
    uint8_t level_transitions[4];     // Exit zones -> target level
} level_info_t;
```

### User Input (Fill-in-the-blank)
When `g_dlg_with_user_input` is set, the dialog expects typed input:
```c
g_dialog_user_input[16]    // Buffer for typed word
```
The 8086 code uses callback cmd=5 (NPC_REPLY) to match the typed word against a list of known words, returning the match index in `g_4bae.x4c82`.

---

## 10. PAX SYSTEM

**File:** `rw_state_pax.c`

### State Machine
```
PS_OPEN_PAX -> Window fold animation -> PS_MAIN_MENU
PS_MAIN_MENU -> Exit -> PS_CLOSE_PAX -> PS_OPEN_PAX -> RWS_NORMAL
PS_MAIN_MENU -> User Info -> PS_USER_INFO -> PS_USER_INFO_WFI -> PS_USER_INFO_END_WFI
PS_MAIN_MENU -> Banking -> PS_BANKING
  PS_BANKING -> Download -> PS_BANK_DOWNLOAD (enter amount)
  PS_BANKING -> Upload -> PS_BANK_UPLOAD (enter amount)
  PS_BANKING -> Transactions -> PS_BANK_TRANSACTIONS_WFI
PS_MAIN_MENU -> News -> PS_NEWS -> PS_NEWS_MENU
  PS_NEWS_MENU -> Select article -> PS_NEWS -> PS_NEWS_WFI -> PS_NEWS_END_WFI
PS_MAIN_MENU -> Bulletin Board -> PS_BOARD_MENU
  PS_BOARD_MENU -> View -> PS_BOARD_VIEW_MENU
    -> Select message -> PS_BOARD_MSG -> PS_BOARD_MSG_WFI -> PS_BOARD_MSG_END_WFI
  PS_BOARD_MENU -> Send -> PS_BOARD_SEND_MSG_ADDRESSEE -> PS_BOARD_SEND_MSG_TEXT
    -> PS_BOARD_SEND_MSG_ACCEPT (Y/N)
```

### Message Visibility System
```c
pax_info_menu_prepare_list():
    For each news/message entry:
        flag & 0x80: Set date from game date_day
        flag & 0x0F:
            0x00: Always show
            0x01: Show if g_4bae.x4bae[addr-0x4BAE] > val
            0x02: Show if g_4bae.x4bae[addr-0x4BAE] != val
            0x03: End of list
```

### Armitage Message Trigger
```c
pax_send_mgs():
    if addressee == "armitage" && text contains "056306118" && !msg_to_armitage_sent:
        g_4bae.msg_to_armitage_sent = 1
        g_4bae.x4c5c = 0
        Add bank transaction: $10000 deposit
```

---

## 11. INVENTORY SYSTEM

**File:** `rw_state_inventory.c`

### State Machine
```
IS_OPEN_INVENTORY -> Window fold animation -> IS_ITEM_LIST
IS_ITEM_LIST -> Select item -> IS_ITEM_OPTIONS
IS_ITEM_OPTIONS -> Exit -> IS_ITEM_LIST
IS_ITEM_OPTIONS -> Discard -> IS_DISCARD_ITEM -> Y/N -> IS_ITEM_LIST
IS_ITEM_OPTIONS -> Operate -> Various outcomes:
  - Nothing happens -> IS_WFI_AND_CONTINUE
  - Hardware failure -> IS_WFI_AND_CONTINUE
  - Program crashed -> IS_WFI_AND_CLOSE
  - Deck (skill chip) -> IS_OPERATE_SOFTWARE_LIST
  - Gas mask toggle -> IS_WFI_AND_CONTINUE
  - Cyberspace item -> IS_WFI_AND_CLOSE
IS_ITEM_OPTIONS -> Erase -> IS_ERASE_SOFTWARE_LIST -> IS_ERASE_SOFTWARE -> Y/N
IS_ITEM_OPTIONS -> Give -> IS_GIVE_CREDITS (enter amount) OR IS_GIVE_ITEM (Y/N)
IS_WFI_AND_* -> Click/Key -> IS_CLOSE_INVENTORY
IS_CLOSE_INVENTORY -> Window fold animation -> RWS_NORMAL
```

### Inventory Data Structure
```c
typedef struct neuro_inventory_t {
    uint8_t items[128];      // 32 items, 4 bytes each
    uint8_t software[128];   // 32 software, 4 bytes each
} neuro_inventory_t;

// Each item: [code, version, damage_flag, unknown]
// 0xFF in code = empty slot
// Credits: code = 0x7F (special)
```

### Item Operation Table
```c
uint8_t g_inventory_item_operations[128];
// Bits 7-6: 0x00=software, 0x80=hardware, 0xC0=skill
// Bits 4-5: 0x00=general, 0x10=database, 0x20=cyberspace
// Bits 0-3: operation code
```

### Item Operation Logic
```c
inventory_operate_item(item):
    if item == credits or op == 0xFF: "Nothing happens"
    if random < item[2]: failure (hardware failure / program crash)
    
    op_type = op & 0xC0
    if hardware (0x80):
        case 0: deck -> show software list (IS_OPERATE_SOFTWARE_LIST)
        case 1: skill chip -> install skill
        case 2: gas mask -> toggle on/off
    if cyberspace only (0x20):
        check if in cyberspace
    if jackable item:
        check if jack available on current level
    if database only (0x10):
        check if in database
```

---

## 12. BODY PARTS SHOP

**File:** `rw_state_body_parts_shop.c`

### State Machine
```
PSS_OPEN -> Window fold animation -> PSS_SELL_MENU or PSS_BUY_MENU
PSS_SELL_MENU -> Select part -> Sell (if not already sold)
PSS_BUY_MENU -> Select part -> Buy (if already sold)
PSS_*_MENU -> Exit -> PSS_CLOSE
PSS_CLOSE -> Window fold animation -> cpu_run() resumes -> RWS_NORMAL
```

### Body Parts Data
```c
char *g_body_parts[20];              // Names
uint16_t g_body_parts_buy_prices[20];    // Buy-back prices
uint16_t g_body_parts_sell_prices[20];   // Sell prices
uint16_t g_body_parts_discounted_prices[20]; // Discounted buy prices
uint16_t g_constitution_damage[20];  // Constitution loss per part

// Sold state: bitstring in g_4bae.sold_body_parts_bitstring[3]
// bit = 1 means sold
```

### Buy/Sell Logic
```c
// Sell:
if part not sold:
    set bit in sold_body_parts_bitstring
    constitution -= constitution_damage[part]
    cash += sell_prices[part]

// Buy:
if part sold AND cash >= price:
    clear bit in sold_body_parts_bitstring
    constitution += constitution_damage[part]
    cash -= buy_prices[part] (or discounted_prices if g_body_shop_discount)
```

---

## 13. SKILLS / ROM / DISK STATES

**Files:** `rw_state_skills.c`, `rw_state_rom.c`, `rw_state_disk_options.c`

### Skills State
Displays installed skills. Reads from `g_3f85.skills[16]` array.

### ROM State
ROM construct management. Checks `g_4bae.x4c25` for ROM availability.

### Disk Options State
Save/Load game. Sets `g_load_game = 1` flag for level reload after load.

---

## 14. CHARACTER CONTROL

**File:** `character_control.c`, `character_control.h`

### Character State
```c
typedef enum character_state_t { CS_IDLE, CS_MOVING };
typedef enum character_dir_t { CD_NULL, CD_UP, CD_RIGHT, CD_DOWN, CD_LEFT };

static character_t g_character = { CS_IDLE, CD_DOWN, 0 };
```

### Animation Frames
```c
// 8 frames per direction, stored in sprite sheet (g_seg013.spritesheet)
uint16_t g_up_frames[8];     // Offsets into sprite sheet
uint16_t g_right_frames[8];
uint16_t g_down_frames[8];
uint16_t g_left_frames[8];
```

### Movement
```c
character_control_update():
    Every 100ms:
        speed_hort_pix = 5, speed_vert_pix = 2
        Move character sprite in current direction
        Animate frames (cycle through 8)
        Check walk bounds (g_8cee[direction])
        
character_control_handle_input(event):
    Mouse: Click in room area -> calculate direction from character to cursor
    Keyboard: Arrow keys -> set direction
    Lock mechanism prevents mouse+keyboard conflict
```

---

## 15. DRAWING SYSTEM

**Files:** `drawing_control.c`, `drawing_control.h`

### Sprite Chain
```c
typedef enum sprite_chain_index_t {
    SCI_CURSOR = 0,
    SCI_NEURO_MENU = 2,
    SCI_DIALOG_BUBBLE = 3,
    SCI_CHARACTER = 4,
    SCI_LEVEL_BG = 9,
    SCI_BACKGRND = 10,
    SCI_TOTAL = 11
} sprite_chain_index_t;

extern sprite_layer_t g_sprite_chain[SCI_TOTAL];
```

### Drawing Functions
```c
drawing_control_add_sprite_to_chain(index, left, top, pixels, flags);
drawing_control_remove_sprite_from_chain(index);
drawing_control_flush_sprite_chain();
drawing_control_draw();    // Composite all sprites to g_vga buffer
```

### VGA Buffer
```c
extern uint8_t g_vga[320 * 200 * 4];  // 320x200, 4bpp or ARGB
```

---

## 16. WINDOW / MENU SYSTEM

**Files:** `neuro_window_control.c`, `neuro_menu_control.c`

### Window System
```c
typedef struct neuro_window_t {
    uint16_t mode;              // NWM_NEURO_UI, NWM_PAX, NWM_INVENTORY, etc.
    uint16_t total_items;
    neuro_button_t *buttons;
    // ...
} neuro_window_t;

extern neuro_window_t g_neuro_window;
extern neuro_window_t g_neuro_windows_pool[4];
```

### Menu System
```c
typedef struct neuro_menu_t {
    uint16_t left, top, right, bottom;
    uint16_t inner_left, inner_top, inner_right, inner_bottom;
    uint16_t mode;
    uint16_t items_count;
    neuro_button_t items[16];
    // ...
} neuro_menu_t;

extern neuro_menu_t g_neuro_menu;
extern neuro_menu_t g_neuro_menus_pool[4];
```

### Button Structure
```c
typedef struct neuro_button_t {
    uint16_t left, top, right, bottom;  // Hitbox
    uint16_t code;                       // Action code
    char label;                          // Keyboard shortcut
} neuro_button_t;
```

---

## 17. RESOURCE MANAGER

**File:** `resource_manager.c`, `resource_manager.h`

### Resource Loading
```c
resource_manager_load_resource("R1.BIH", buffer);  // Load from DAT files
resource_manager_load_resource("TITLE.IMH", buffer);
resource_manager_load_resource("SAVEGAME.SAV", buffer);
resource_manager_write_resource("SAVEGAME.SAV", buffer);
```

Resources are stored in `.DAT` archive files and decompressed on load using Huffman + RLE decompression from `LibNeuroRoutines`.

---

## 18. SAVE/LOAD SYSTEM

**File:** `save_load.c`, `save_load.h`

### Save Format
```c
#pragma pack(push, 1)
typedef struct neuro_savegame_t {
    x3f85_t x3f85;                    // VM states, inventory, skills
    x4bae_t x4bae;                    // Player state
    uint8_t visited_levels[8];        // Visited rooms bitstring
    uint8_t x3b94[374];               // Misc state
    neuro_menu_t neuro_menus[4];      // Menu state
    neuro_window_t neuro_windows[4];  // Window state
} neuro_savegame_t;                    // ~3000 bytes per slot
```

Save file: `SAVEGAME.SAV`, 4 slots x 3000 bytes = 12000 bytes.

### Save/Load Functions
```c
save_game(slot):
    Copy g_3f85, g_4bae, visited_levels, sprite_chain, menus, windows
    Write to SAVEGAME.SAV

load_game(slot):
    Read from SAVEGAME.SAV
    Restore all state
    Reset sprite chain and menus
    g_4bae.x4bcc = 1  // Mark as loaded
```

---

## 19. ADDRESS TRANSLATION

**File:** `address_translator.c`, `address_translator.h`

Translates 16-bit segment:offset addresses to 64-bit linear pointers:

```c
uint8_t* translate_x16_to_x64(uint16_t seg, uint16_t offt);
void translate_x64_to_x16(uint8_t *src, uint16_t *seg, uint16_t *offt);

// Segment constants:
#define SEG_000 0 ... SEG_016 16
#define DSEG    17  // Data segment
```

This allows the 8086 emulator to reference data in the host's memory space.

---

## 20. LIBNEUROROUTINES

**File:** `LibNeuroRoutines/neuro_routines.h`

### Decompression
```c
decompress_imh(src, dst);   // IMH image files
decompress_pic(src, dst);   // PIC backdrop files
decompress_bih(src, dst);   // BIH room data files
decompress_anh(src, dst);   // ANH animation files
decompress_txh(src, dst);   // TXH text files
huffman_decompress(src, dst);
decode_rle(src, len, dst);
```

### Drawing
```c
build_character(c, dst);          // Render single CP437 character
build_string(string, w, h, l, t, dst);  // Render string to pixel buffer
build_text_frame(h, w, dst);     // Build text frame header
```

### Sound
```c
build_track_waveform(track_num, waveform, len);
```

### Background Animation
```c
bg_animation_init_tables(tables, decompd_anh);
bg_animation_update(tables, animations, working_area, bg_pixels);
```

### Font
```c
extern uint8_t cp437_font[1024];  // CP437 character set (8x8 pixels)
```

---

## 21. JAVA <-> WIN64 MAPPING GUIDE

This table maps Java concepts to their Win64 equivalents:

| Java Concept | Java File | Win64 Equivalent | Win64 File |
|-------------|-----------|-----------------|------------|
| GameState | `GameState.java` | `g_4bae` + `g_3f85` | `data.h` |
| Game Loop | `NeuroGamePane.loop()` | `main.c` game loop | `main.c` |
| Modes | `NeuroModePane` subclasses | `neuro_scene_t` | `scene_control.h` |
| Room System | `Room.java`, `RoomMap.java` | Level system, `g_4bae.level_n`, `level_info[58]` | `data.h` |
| Room Extras | `R<N>Extras.java` | BIH bytecode + 8086 code | `R<N>.BIH` |
| Dialog System | `DialogPopup.java` | `rw_state_dialog.c` state machine | `rw_state_dialog.c` |
| Dialog Commands | `DialogCommand.java` | BIH opcodes 0x05-0x08 (JE/JNE/JL/JGE) + 8086 code | `opcodes.h`, `scene_real_world.c` |
| Inventory | `InventoryPopup.java` | `rw_state_inventory.c` state machine | `rw_state_inventory.c` |
| Items | `Item.java` catalog | `g_inventory_item_operations[128]`, item codes | `data.h` |
| Skills | `Skill.java` | `g_3f85.skills[16]` | `data.h` |
| Body Parts | `BodyPart.java` | `g_body_parts[]`, price arrays | `rw_state_body_parts_shop.c` |
| Warez/Software | `Warez.java` | `g_3f85.inventory.software[]` | `data.h` |
| Deck Items | `DeckItem.java` | Handled via item operations (op=deck) | `rw_state_inventory.c` |
| PAX System | `PaxPopupPane.java` | `rw_state_pax.c` state machine | `rw_state_pax.c` |
| Banking | `PaxBankingNode.java` | `rw_state_pax.c` banking states | `rw_state_pax.c` |
| Save/Load | `GameStateUtils.java` | `save_load.c` (binary copy) | `save_load.c` |
| Resource Loading | `ResourceManager.java` | `resource_manager.c` | `resource_manager.c` |
| Character Movement | `RoomPane.tick()` | `character_control.c` | `character_control.c` |
| Room Position | `RoomPosition.java` | `g_seg016.roompos[]` | `data.h` |
| Room Bounds | `RoomBounds.java` | `g_8cee[4][4]` (walk area bounds) | `data.h` |
| AI System | `AI.java` | **Not implemented** in Win64 | N/A |
| Cyberspace | `CyberspacePopup.java` | **Not implemented** in Win64 | N/A |
| Database System | `Database.java` | **Not implemented** in Win64 | N/A |
| Music Manager | `MusicManager.java` | `build_track_waveform()` stub | `LibNeuroRoutines` |
| Animations | `AnimationEntry.java` | `bg_animation_control.c` | `bg_animation_control.c` |
| Text Rendering | JavaFX Text | `build_string()` + CP437 font | `LibNeuroRoutines` |

### Key Architectural Differences

| Aspect | Java | Win64 |
|--------|------|-------|
| Game Logic | Reimplemented in Java | Runs in 8086 emulator + BIH bytecode |
| Room Behavior | `R<N>Extras` Java classes | BIH bytecode programs + 8086 machine code |
| Dialog Flow | `int[][] dialogChain` | BIH text strings + VM opcode 0x01/0x17/0x18 |
| NPC Response | `askWord1/2()` Java methods | 8086 callback cmd=5 (NPC_REPLY) |
| Item Effects | `Warez.use()`, `Skill.use()` | 8086 machine code + item operation table |
| Data Format | Java Properties files | Binary binary dump of memory state |
| UI Framework | JavaFX | SFML + custom drawing |

---

## 22. INDEX: FILE -> FUNCTION MAP

### NeuromancerWin64/

| File | Key Functions | Purpose |
|------|-------------|---------|
| `main.c` | `main()`, `update_cursor()` | Entry point, game loop |
| `neuro86.c` | `cpu_new()`, `cpu_reset()`, `cpu_run()`, `cpu_destroy()` | 8086 CPU emulator |
| `opcodes.h` | `add_u16`, `sub_u16`, `cmp_u16`, `tbl_opcodes[]` | 8086 instruction implementations |
| `scene_control.c` | `scene_control_setup_scene()` | Scene manager |
| `scene_main_menu.c` | `setup_main_menu_scene()`, `main_menu_handle_*()` | Title screen |
| `scene_real_world.c` | `setup_real_world_scene()`, `neuro_vm()`, `sub_105F6()`, `neuro_cb()` | Main game scene, VM, callbacks |
| `rw_state_dialog.c` | `handle_dialog_input()`, `update_dialog()` | NPC dialog state |
| `rw_state_inventory.c` | `handle_inventory_input()`, `update_inventory()`, `inventory_operate_item()` | Inventory state |
| `rw_state_pax.c` | `handle_pax_input()`, `update_pax()`, `pax_send_mgs()` | PAX system |
| `rw_state_skills.c` | `handle_skills_input()`, `update_skills()` | Skills display |
| `rw_state_rom.c` | `handle_rom_input()`, `update_rom()` | ROM management |
| `rw_state_disk_options.c` | `handle_disk_options_input()`, `update_disk_options()` | Save/load disk |
| `rw_state_body_parts_shop.c` | `handle_parts_shop_input()`, `update_parts_shop()` | Body shop |
| `character_control.c` | `character_control_add_sprite_to_chain()`, `character_control_update()`, `character_control_handle_input()` | Player movement |
| `bg_animation_control.c` | `bg_animation_control_init_tables()`, `bg_animation_control_update()`, `bg_animation_control_prepare()` | Background animations |
| `drawing_control.c` | `drawing_control_add_sprite_to_chain()`, `drawing_control_remove_sprite_from_chain()`, `drawing_control_draw()` | Sprite rendering |
| `neuro_window_control.c` | `neuro_window_setup()`, `neuro_window_add_button()`, `neuro_window_draw_string()`, `neuro_window_handle_input()`, `restore_window()` | Window system |
| `neuro_menu_control.c` | `neuro_menu_create()`, `neuro_menu_add_item()`, `neuro_menu_draw_text()`, `neuro_menu_handle_input()`, `neuro_menu_destroy()` | Menu system |
| `resource_manager.c` | `resource_manager_load_resource()`, `resource_manager_write_resource()` | DAT resource loading |
| `save_load.c` | `save_menu()`, `load_menu()`, `save_game()`, `load_game()` | Save/load |
| `address_translator.c` | `translate_x16_to_x64()`, `translate_x64_to_x16()` | Address translation |
| `data.c` | `g_4bae`, `g_3f85`, `g_seg004`, `g_seg016`, etc. | Global data definitions |
| `items.c` | `get_item_name()`, `count_items()` | Item name lookup |
| `window_animation.c` | `window_animation_setup()`, `window_animation_update()` | Window animations (fold/unfold, fade, text scroll) |
| `utilities.c` | `extract_line()`, `sfHandleTextInput()`, `ascii_toSfKeyCode()` | Utility functions |
| `not_implemented.c` | Placeholder functions | Stub implementations |

### LibNeuroRoutines/

| File | Key Functions | Purpose |
|------|-------------|---------|
| `decompression.c` | `decompress_imh()`, `decompress_pic()`, `decompress_bih()`, `decompress_anh()`, `decompress_txh()` | Resource decompression |
| `huffman_decompression.c` | `huffman_decompress()` | Huffman decode |
| `drawing.c` | `build_character()`, `build_string()`, `build_text_frame()` | Text rendering |
| `animation.c` | `bg_animation_init_tables()`, `bg_animation_update()` | BG animation engine |
| `audio.c` | `build_track_waveform()` | Sound generation (stub) |
| `cp437.c` | `cp437_font[]` | Font data |
| `resources_lists.c` | `g_res_imh[]`, `g_res_pic[]`, `g_res_bih[]`, etc. | Resource tables |

---

## APPENDIX: NOT YET IMPLEMENTED IN WIN64

The following game systems present in the original DOS game and the Java port are **not yet implemented** in the Win64 port:

1. **Cyberspace** - The matrix/cyberspace exploration and battle system
2. **AI Battles** - AI encounters and combat
3. **Database System** - Link code connection, password entry, message boards
4. **Warez Effects** - Most warez have stub implementations only
5. **Music/Sound** - Sound generation is a stub
6. **Deck System** - Deck hardware management is partial

The `scene_not_implemented.cpp` serves as a placeholder for these unimplemented features.

---

*This manual is based on the Reuromancer codebase at `/data/nfs/dos/GAMES/8/NEURO/Reuromancer/`.
All file paths are relative to that directory.*