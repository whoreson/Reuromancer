# Neuromancer Java Port -- Comprehensive Reference Manual

> For developers creating a C port from the Java implementation (Javamancer).
> Every major subsystem is documented with Java file references, class/method signatures,
> algorithmic logic, and data-flow descriptions.

---

## TABLE OF CONTENTS

1. [Architecture Overview](#1-architecture-overview)
2. [Game Loop & Mode System](#2-game-loop--mode-system)
3. [Game State](#3-game-state)
4. [Room System](#4-room-system)
5. [Dialog System](#5-dialog-system)
6. [Inventory & Items](#6-inventory--items)
7. [Skills](#7-skills)
8. [Warez (Software)](#8-warez-software)
9. [Deck System](#9-deck-system)
10. [Body Parts](#10-body-parts)
11. [Cyberspace System](#11-cyberspace-system)
12. [Database System](#12-database-system)
13. [AI System](#13-ai-system)
14. [PAX System](#14-pax-system)
15. [Save/Load System](#15-save--load-system)
16. [Resource Management](#16-resource-management)
17. [Audio System](#17-audio-system)
18. [UI / Popup System](#18-ui--popup-system)
19. [Index: Java File -> Function Map](#19-index-java-file---function-map)

---

## 1. ARCHITECTURE OVERVIEW

The game is structured in three layers: **Model**, **View**, and **Controller** (loosely MVC).

| Layer | Java Package | Responsibility |
|-------|-------------|----------------|
| **Model** | `com.maehem.javamancer.neuro.model` | All game data: GameState, Items, Skills, Warez, AI, Database, Room definitions |
| **View** | `com.maehem.javamancer.neuro.view` | All rendering: Modes (Title, Room, EndGame), Popups, Cyberspace panes, Databases, PAX |
| **Resource** | `com.maehem.javamancer.resource` | DAT file parsing, image/sound loading, Huffman decompression, animation handling |
| **Root App** | `com.maehem.javamancer` | Top-level application shell, settings, resource browser |

**Key design principle for C port:** The model layer is entirely UI-agnostic and can be ported directly. The view layer depends on JavaFX and must be reimplemented with your chosen graphics/input library (e.g., SDL, Allegro).

---

## 2. GAME LOOP & MODE SYSTEM

### 2.1 Game Loop

**File:** `NeuroGamePane.java`

The main loop runs at **15 FPS** (every 66ms), implemented via `AnimationTimer`:

```
loop():
    if gameState.loadSlot > 0:
        loadSlot(gameState.loadSlot)  // Load saved game, switch to RoomMode
        return
    if gameState.requestQuit:
        fire Action.QUIT
    if gameState.useDoor == Door.EPILOGUE:
        setMode(EndGameMode)
        return
    if gameState.useDoor != Door.NONE:
        // Room transition
        gameState.roomPrevious = gameState.room
        gameState.room = RoomMap.getRoom(gameState.room, gameState.useDoor)
        setMode(new RoomMode)
        Set player position from RoomPosition table
    else if !gameState.pause:
        if ++frameCount > 15:   // Every 1 second (15 ticks)
            gameState.addMinute()     // Advance game time
            gameState.updateConstitution()  // Heal damage
            mode.updateStatus()
            frameCount = 0
        mode.tick()
```

### 2.2 Modes (NeuroModePane subclasses)

| Mode | File | Trigger | Description |
|------|------|---------|-------------|
| `TitleMode` | `TitleMode.java` | Application start | Title screen, New Game / Load / Quit |
| `RoomMode` | `RoomMode.java` | New game or room transition | Main gameplay: room rendering, button bar, popups |
| `EndGameMode` | `EndGameMode.java` | Epilogue door or game win | Final credits/epilogue |

**Mode transition:** `setMode(newMode)` calls `oldMode.destroy()`, clears children, adds new mode, calls `newMode.initCursor()`, and sets `gameState.pause = false`.

### 2.3 Action events

**Interface:** `NeuroModePaneListener`

```
enum Action {
    QUIT, LOAD, NEW_GAME, EPILOGUE, MUTE_MUSIC
}
neuroModeActionPerformed(Action action, Object[] actionObjects)
```

Modes fire actions via `getListener().neuroModeActionPerformed(...)`.

---

## 3. GAME STATE

**File:** `GameState.java`

`GameState` is the single source of truth for all mutable game data. It holds:

### 3.1 Player identity & health
```
String name                          // Default "Case", max 12 chars
int damage                           // Current damage (heals at 10/tick when not jacked)
static int CONSTITUTION_MAX = 2000
static int CONSTITUTION_HEAL_RATE = 10
int getConstitution() = getConstitutionUsable() - damage
int getConstitutionUsable() = CONSTITUTION_MAX - sum(soldBodyParts.constDamage)
void updateConstitution()            // Heals 10 per tick if damage > 0 and not jacked
void applyEnemyAttack(int amount)    // Adds to damage; sets flatline if constitution <= 0
void revive()                        // Sets damage = usable-10, money=0, door=BODY_SHOP
```

### 3.2 Time/Date
```
int dateMinute (0), dateHour (12), dateDay (16), dateMonth (11), dateYear (2058)
void addMinute()                     // Increments time, handles rollover.
                                     // Deducts 1 credit per minute if deck active and not free.
String getDateString()               // Returns "MM/DD/YY"
```

### 3.3 Money
```
int moneyChipBalance = 6             // Credits on chip
int bankBalance = 2000               // FOBS bank
int bankZurichBalance = 0
int bankGemeinBalance = 30000
int bankBerneBalance = 500000
ArrayList<BankTransaction> bankTransactionRecord
String PLAYER_BAMA = "056306118"
String BANK_ZURICH_ID = "712345450134"
String BANK_GEMEIN_ID = "646328356481"
String BANK_BERNE_AUTH_CODE = "LYMA1211MARZ"
String BANK_BERNE_ID = "121519831200"
```

### 3.4 Inventory
```
ArrayList<Item> inventory            // Player's carried items
ArrayList<Skill> skills              // Installed skills
ArrayList<BodyPart> soldBodyParts    // Body parts sold (reduces max constitution)
ArrayList<Warez> software            // Warez on current deck
int deckSlots                        // Slots on current deck
```

### 3.5 Deck/Matrix state
```
DeckItem usingDeck                   // Currently active deck
DeckItem lastUsedDeck
boolean usingDeckErase               // Ephemeral: erase mode
int matrixPosX = 112, matrixPosY = 96  // Current position in cyberspace grid
int romInstalled = -1                // -1 = none
Database database                    // Currently targeted database (ephemeral)
boolean databaseBattle               // In database battle (ephemeral)
boolean databaseBattleBegin          // Battle just started (ephemeral)
boolean databaseArrived              // Arrived at DB (ephemeral)
```

### 3.6 Room state
```
Room room                            // Current room
Room roomPrevious
int roomPosX = 160, roomPosY = 90    // Player position in room
RoomBounds.Door useDoor = NONE       // Door to use for next room transition
```

### 3.7 Quest/flag state
```
// Cheap Hotel
int hotelCharges = 1000, hotelOnAccount = 0
int hotelCaviar = 1, hotelSake = 2
int hotelDeliverCaviar = 0, hotelDeliverSake = 0

// Massage Parlor
boolean massageInfo1-5 = false

// Various flags
boolean bbsMsgFromArmitageRead, bbsMsgToArmitageSent, ratzPaid
boolean shivaChipMentioned, shivaGaveChip, joystickGiven, gasMaskIsOn
boolean bodyPartDiscount, bankZurichRobbed
boolean comlink2recieved, comlink6uploaded, asanoDiscount
boolean securityPassGiven, larryMoeWanted, hitachiVolunteer
int psychoProbeCount = 0
int hosakaDaysSincePaid = -1         // -1 = unconfigured
```

### 3.8 Lists
```
ArrayList<Person> seaWantedList, chibaWantedList, hosakaEmployeeList
ArrayList<Room> visited              // Rooms fully read (private)
ArrayList<Room> dialogAllowed        // Rooms where NPC can be talked to
ArrayList<Database> defeatedDbList
ArrayList<AI> defeatedAiList
ArrayList<BbsMessage> messageSent
ArrayList<NewsArticle> news
ArrayList<BbsMessage> bbs            // PAX BBS messages
```

### 3.9 Key methods
```
GameState(ResourceManager rm)                          // Constructor
void initNewGame()                                     // Initialize starting state
boolean roomCanTalk()                                  // dialogAllowed.contains(room)
void setRoomTalk(boolean val)                          // Add/remove room from dialogAllowed
boolean hasVisited(Room room)
boolean setVisited(Room room)
boolean hasInventoryItem(Item/Catalog checkItem)
Item getInventoryItem(Catalog)
boolean removeInventoryItem(Catalog)
boolean addSoftware(Warez w)                           // Adds to deck if slot available
boolean eraseSoftware(Warez w)
boolean hasInstalledSoftware(Warez w)
boolean hasInstalledSkill(SkillItem)
Skill getInstalledSkill(SkillItem)
void battleStart()                                     // Start DB battle from cyberspace
AI getAI(Class<? extends AI>)
boolean isAiDefeated(Class<? extends AI>)
void cleanUp()                                         // Stop music on game end/load
void loadSlot(int i) / saveSlot(int i)                 // Trigger save/load
```

---

## 4. ROOM SYSTEM

### 4.1 Room Enum

**File:** `Room.java`

```
enum Room {
    R1("Chatsubo", R1Extras.class),
    R2("Street Chatsubo", R2Extras.class),
    ...
    R58("Musabori Headquarters", R58Extras.class);

    final String roomName;
    final Class<? extends RoomExtras> extraClazz;
    RoomExtras extras;
    ArrayList<Door> locked;       // Locked doors

    int getIndex()                // Returns 0-based index
    RoomExtras getExtras()        // Lazy instantiation via reflection
    boolean hasJack()             // Has cyberspace jack
    JackZone getJack()            // Returns JackZone
    boolean hasPax()              // Has PAX terminal
    void lockDoor(Door d)
    void unlockDoor(Door d)
    boolean isDoorLocked(Door d)
    static Room lookup(String name)
}
```

### 4.2 Room Map

**File:** `RoomMap.java`

Defines connections between rooms (Top, Right, Bottom, Left):

```
enum RoomMap {
    R1(Room.R1, null, null, Room.R2, null),  // Top, Right, Bottom, Left
    R2(Room.R2, Room.R1, Room.R5, null, null),
    ...
    
    static Room getRoom(Room r, RoomBounds.Door d)
}
```

Special doors: JAIL->R3, STREET_CHAT->R2, BODY_SHOP->R4, SHUTTLE->R10/R28,
SPACEPORT->R19, FREESIDE->R29, ZION->R11, BEACH->R50

### 4.3 Room Bounds

**File:** `RoomBounds.java`

Defines walkable rectangle and door positions per room:

```
enum RoomBounds {
    R1(t=184, r=580, b=224, l=40, tx=0, tw=0, ry=0, rw=0, bx=436, bw=144, ly=0, lw=0),
    ...
    
    enum Door { NONE, TOP, RIGHT, BOTTOM, LEFT, JAIL, STREET_CHAT, BODY_SHOP,
                SHUTTLE, FREESIDE, ZION, SPACEPORT, BEACH, EPILOGUE }
    
    static RoomBounds get(Room room)
}
```

### 4.4 Room Positions

**File:** `RoomPosition.java`

Defines player spawn and NPC position per room:

```
enum RoomPosition {
    R1(playerX=300, playerY=204, npcX=120, npcY=40),
    R2(playerX=420, playerY=204, npcX=110, npcY=150),
    ...
    DEFAULT(300, 204, 0, 0);
    
    static RoomPosition get(Room room)
}
```

### 4.5 Room Extras (Room-Specific Logic)

**File:** `RoomExtras.java` (abstract base)

Every room with NPC interaction has an `R<#>Extras` subclass:

```
abstract class RoomExtras {
    // Dialog chain
    abstract int[][] getDialogChain()     // Array of [dialog_index, ...]
    int dialogWarmUp(GameState gs)        // Initial dialog index (default: 2)
    void dialogNoMore(GameState gs)       // Disable dialog for room
    
    // Room initialization
    abstract void initRoom(GameState gs)  // Called on room entry
    
    // Jack/PAX
    JackZone jackZone()                   // Returns null if no jack
    boolean hasPAX()                      // Returns false
    
    // Vending
    ArrayList<SkillItem> getVendSkillItems(GameState gs)
    ArrayList<Item> getVendItems(GameState gs)
    ArrayList<SoftwareItem> getVendSoftwareItems(GameState gs)
    ArrayList<SkillItem> getUpgradeSkillItems(GameState gs)
    boolean onVendItemsFinished(GameState, boolean purchased)
    boolean onSkillVendFinished(GameState)
    
    // Item give/receive
    boolean give(GameState gs, Item item, int aux)    // Player gives item to NPC
    boolean getItem(GameState gs, Item item)           // Player receives item from NPC
    
    // Word queries (fill-in-the-blank dialogs)
    int askWord1(GameState, String word)    // Returns dialog index or -1
    int askWord2(GameState, String word)
    
    // Hooks
    int onDialogIndex(GameState, int index)          // Called before showing dialog
    int onDialogPreCommand(GameState, DialogCommand)  // Called before command
    void onDialogPostCommand(GameState, DialogCommand) // Called after command
    int[] onFilter1(GameState)                         // Dynamic dialog options
    boolean onPopupExit(GameState, PopupPane)
    boolean onDeckFinished(GameState)
    
    // Discount
    void applyDiscount(GameState)
    int getDiscount(GameState)
    int getSkillDiscount(GameState)
    
    // Tick
    void tick(GameState gs)        // Called every frame when no popup
    boolean isRequestDialogPopup() // Flag to trigger dialog popup
    void setRequestDialogPopup(boolean)
}
```

### 4.6 Room Extras per Room (Summary)

| Room | File | Key Features |
|------|------|-------------|
| R1 (Chatsubo) | `R1Extras.java` | Armitage NPC, mission briefing |
| R4 (Body Shop) | `R4Extras.java` | Buy/sell body parts, revive |
| R7 (Cheap Hotel) | `R7Extras.java` | Room service, bill, PAX, jack=ZERO |
| R12 (Larry's Software) | `R12Extras.java` | Sell CopTalk, Panther Moderns meeting setup |
| R17 (Street Maas) | `R17Extras.java` | Shiva NPC, chip give |
| R19 (Spaceport) | `R19Extras.java` | Ticket purchase (Freeside $1000, Zion $500) |
| R22 (Villa Straylight) | `R22Extras.java` | Neuromancer encounter, jack=SEVEN |
| R24 (Massage Parlor) | `R24Extras.java` | Buy info ($20), police bot animation |
| R25 (Shin's Pawn) | `R25Extras.java` | Buy/sell UXB deck, pawn ticket |
| R40 (Crazy Edo) | `R40Extras.java` | Caviar/sake delivery, ComLink 2.0 |
| R44 (Asano's) | `R44Extras.java` | Deck vendor, 20% discount |
| R45 (Security Robot) | `R45Extras.java` | Robot encounter |
| R46 (Matrix Restaurant) | `R46Extras.java` | Shiva NPC, restaurant pass |
| R51 (Fuji Electric) | `R51Extras.java` | Jack, wanted list |
| R52 (Security Gate) | `R52Extras.java` | Gas mask check |
| R53 (Hitachi) | `R53Extras.java` | Lung removal experiment, jack=TWO |
| R56 (SenseNet) | `R56Extras.java` | Security pass, ROM checkout, jack=FOUR |
| R57 (Hosaka) | `R57Extras.java` | Paycheck, employee list, jack=TWO |
| R58 (Musabori) | `R58Extras.java` | Jack=TWO, no dialog |

### 4.7 Room Rendering

**File:** `RoomPane.java`

- Renders room backdrop (PIC image)
- Draws walkable bounds rectangle (yellow, semi-transparent)
- Draws door rectangles (red, semi-transparent)
- Player node walks toward clicked position
- Door collision triggers `gameState.useDoor`
- Animation entries (ANH files) start/stop based on animation flags

```
RoomPane.tick(GameState):
    processWalk(gs)             // Move player toward walk target
    updateDoors(gs)             // Check door collisions
    Check animation flags and start/stop animations
```

---

## 5. DIALOG SYSTEM

### 5.1 Dialog Command Codes

**File:** `DialogCommand.java`

```
enum DialogCommand {
    DESC(50),        // Show text in room description
    LONG_DESC(51),   // Show long room description
    SHORT_DESC(52),  // Show short room description
    NPC(53),         // Don't toggle to PLAYER after
    PLAYER(54),      // Don't toggle to NPC after
    WORD1(55),       // Fill-in-the-blank (calls askWord1)
    WORD2(56),       // Fill-in-the-blank (calls askWord2)
    WHERE_IS(57),    // Street Light Girl query
    DISCOUNT(58),    // Apply vendor discount
    LUNGS(60),       // Lungs removed at Hitachi
    BODY_SELL(61),   // Open body shop sell popup
    BODY_BUY(62),    // Open body shop buy popup
    SKILL_SELL(63),  // Skill sell menu
    SKILL_BUY(64),   // Skill buy menu (Larry)
    SKILL_UPGRADE(65), // Skill upgrade menu
    INFO_BUY(66),    // Buy info (massage parlor)
    ITEM_BUY(67),    // Buy item from NPC
    ITEM_GET(68),    // Receive item directly
    SOFTWARE_BUY(69), // Buy software
    EXIT_T(70) through EXIT_L(73), // Exit room directions
    EXIT_ST_CHAT(74), // Exit to street outside Chatsubo
    EXIT_BDSHOP(75), // Exit to body shop
    EXIT_SHUTTLE_FS(76), // Freeside shuttle
    EXIT_SHUTTLE_ZION(77), // Zion shuttle
    EXIT_X(78),      // Exit determined by code
    DEATH(79),       // Go to jail
    TO_JAIL(80),     // Go to jail
    DECK_WAIT(81),   // Wait for deck exit
    MASSAGE_BOT(82), // Trigger police bot
    UXB(90),         // Shin gives UXB deck
    PASS(91),        // Shiva gives restaurant pass
    CAVIAR(92),      // Edo gives ComLink 2.0
    CHIP(94),        // N credits
    FINE_BANK_500(95), // Fine $500
    FINE_BANK_20K(96), // Fine $20000
    DIALOG_NO_MORE(97), // End dialog but keep open
    DIALOG_CLOSE(98),    // Close dialog popup
    DIALOG_END(99),      // End dialog, close popup
    ON_FILTER_1(400),    // Dynamic dialog options
    DESC_DIRECT(500)     // Print text[index-500] into description
}
```

### 5.2 Dialog Popup

**File:** `DialogPopup.java`

```
class DialogPopup extends DialogPopupPane {
    enum Mode { NPC, PLAYER }
    
    int dialogIndex              // Current dialog text index
    int dialogSubIndex           // Current sub-index (player response choice)
    int dialogCountDown          // Auto-advance timer
    int[][] dialogChain          // From RoomExtras.getDialogChain()
    
    // Flow:
    // 1. dialogWarmUp() returns initial index
    // 2. NPC speaks (index from dialogChain)
    // 3. Player selects response (advance subIndex)
    // 4. If value < 50: player says text, NPC responds
    // 5. If value >= 50: execute DialogCommand
    // 6. WORD1/WORD2: player types word, askWord1/2 returns new index
    
    void dialogCounter()         // Called each tick; advances dialog
    void processCommand(DialogCommand)  // Executes command logic
}
```

Dialog chain format: `int[][]` where each row is a dialog node. Each element is either a text index (<50) or a command (>=50).

Example:
```
{LONG_DESC.num}, {SHORT_DESC.num},     // 0, 1 - Room descriptions
{3, 4, 5, 6},                          // 2 - NPC opening, 4 responses
{8},                                   // 3 - NPC response to choice 1
{9},                                   // 4 - NPC response to choice 2
{BODY_SELL.num},                       // 5 - Opens sell popup
{DIALOG_CLOSE.num},                    // 6 - Closes dialog
```

### 5.3 Word Fill-In

When a dialog text contains `@---------------` and the next command is WORD1 or WORD2:
1. Player types a word
2. `RoomExtras.askWord1/2()` is called with the word
3. Returns a dialog index (typically from a Map lookup)
4. If -1, the word is not recognized

---

## 6. INVENTORY & ITEMS

### 6.1 Item Catalog

**File:** `Item.java`

```
enum Catalog {
    // Warez items (0-65)
    MIMIC(0, "Mimic", MimicWarez.class),
    JAMMIES(1, "Jammies", JammiesWarez.class),
    ...
    RECEIPTFORGER(61, "Receipt Forger", ReceiptForgerWarez.class),
    
    // Skills (67-82)
    BARGANING(67, "Bargaining", SkillItem.class, BargainingSkill.class),
    COPTALK(68, "CopTalk", SkillItem.class, CopTalkSkill.class),
    ...
    ZEN(81, "Zen", SkillItem.class, ZenSkill.class),
    MUSICIANSHIP(82, "Musicianship", SkillItem.class, MusicianshipSkill.class),
    
    // Body/Real items (83-103)
    CYBEREYES(83, "CyberEyes", DeckItem.class),
    GUESTPASS(86, "guest pass", RealItem.class),
    JOYSTICK(89, "joystick", RealItem.class),
    CAVIAR(94, "caviar", RealItem.class),
    PAWNTICKET(95, "pawn ticket", RealItem.class),
    SECURITYPASS(96, "Security Pass", RealItem.class),
    ...
    
    // Special
    CREDITS(666, "Credits", CreditsItem.class),
    NONE(999, "none", Object.class);

    final int num;
    final String itemName;
    final Class clazz;          // Instantiable class (Warez, SkillItem, RealItem, etc.)
    final Class skillClazz;     // For skills: the Skill class to instantiate
}
```

### 6.2 Item Types

| Type | Class | Description |
|------|-------|-------------|
| Warez | `Warez` subclass | Software programs for deck |
| Skill | `SkillItem` -> `Skill` subclass | Installable skills |
| Deck | `DeckItem` subclass | Deck hardware |
| Real | `RealItem` | Physical items (caviar, passes, etc.) |
| Credits | `CreditsItem` | Money chip |

### 6.3 Inventory Popup

**File:** `InventoryPopup.java`

```
Modes: MENU, EFFECT, INSTALL, INSTALL_SUMMARY, DISCARD, DISCARD_SUMMARY, ASK_GIVE, CREDITS

Menu: Shows 4 items at a time, prev/next navigation
Item Options: Operate, Discard, Give, Erase (for decks)
Install Skill: Y/N confirmation, then SkillItem.installSkillItem()
Give Item: Y/N confirmation, then RoomExtras.give() + remove from inventory
Give Credits: Enter amount, verify chip balance, give via RoomExtras.give()
Discard: Y/N confirmation, remove from inventory
Operate Deck: Sets gameState.usingDeck, opens DeckPopup
```

---

## 7. SKILLS

### 7.1 Skill Base Class

**File:** `Skill.java`

```
abstract class Skill {
    final Item.Catalog catalog;
    int level;
    final int MAX_LEVEL;
    
    abstract String getDescription()
    abstract void use()
    int upgrade()           // level++ capped at MAX_LEVEL
    int getRunDuration()    // 0 = instant, override for over-time
    int getEffect(GameState gs)    // Damage/effect per tick (default 0)
    void putProps(String prefix, Properties p)   // Save
    void pullProps(String prefix, Properties p)  // Load
    static Skill getInstance(Catalog item, int level)  // Reflection factory
}
```

### 7.2 Skill List

| Skill | File | Max Level | AI Battle | Effect |
|-------|------|-----------|-----------|--------|
| Bargaining | `BargainingSkill.java` | - | No | Vendor discounts |
| CopTalk | `CopTalkSkill.java` | - | No | Police evasion |
| Warez Analysis | `WarezAnalysisSkill.java` | - | No | Software info |
| Debug | `DebugSkill.java` | - | No | Repair software |
| Hardware Repair | `HardwareRepairSkill.java` | - | No | Repair decks |
| ICE Breaking | `IceBreakingSkill.java` | - | No | ICE combat bonus |
| Evasion | `EvasionSkill.java` | - | No | Police evasion |
| Cryptology | `CryptologySkill.java` | - | No | Decrypt messages |
| Japanese | `JapaneseSkill.java` | - | No | Japanese dialogue |
| **Logic** | `LogicSkill.java` | - | **Yes** | AI damage |
| Psychoanalysis | `PsychoanalysisSkill.java` | - | **Yes** | Reveal AI weakness |
| **Phenomenology** | `PhenomenologySkill.java` | - | **Yes** | AI damage |
| **Philosophy** | `PhilosophySkill.java` | - | **Yes** | AI damage |
| **Sophistry** | `SophistrySkill.java` | - | **Yes** | AI damage |
| Zen | `ZenSkill.java` | - | **Yes** | Heal CON in cyberspace |
| Musicianship | `MusicianshipSkill.java` | - | No | Music-related |

**AI Battle Skills:** Philosophy, Sophistry, Phenomenology, Logic (damage), Psychoanalysis (reveal weakness), Zen (heal).

### 7.3 Skill Installation

**File:** `SkillItem.java`

```
static Skill installSkillItem(SkillItem skillItem, ArrayList<Skill> skills) {
    // Check if already installed
    for (Skill s : skills) {
        if (s.getClass().equals(skillItem.item.skillClazz)) return null;  // Already have it
    }
    Skill skill = Skill.getInstance(skillItem.item, 1);
    skills.add(skill);
    return skill;
}
```

### 7.4 Skill Popup

**File:** `SkillsPopup.java`

Shows installed skills with level and description.

---

## 8. WAREZ (SOFTWARE)

### 8.1 Warez Base Class

**File:** `Warez.java`

```
abstract class Warez {
    final Item.Catalog item;
    int version;
    boolean damaged;
    int runRemaining;              // Timer for duration effects
    
    static String USE_OK = "OK"
    static String REQUIRES_ICE = "Requires ICE."
    static int RUN_FOREVER = Integer.MAX_VALUE
    
    String use(GameState gs)        // Pre-use check (default: USE_OK)
    void start()                    // Set runRemaining = getRunDuration()
    void finish(GameState gs)       // Call abort + finished handler
    void abort(GameState gs)        // Stop, no handler (Virus/Corruptor override to self-delete)
    void tick(GameState gs)         // Decrement runRemaining, call finish when done
    
    abstract int getRunDuration()   // ms, or RUN_FOREVER
    abstract int getEffect(GameState gs)  // Damage per tick
    
    String getMenuString()          // "Name  V"
    void putProps/pullProps(...)    // Save/Load
    static Warez getInstance(Catalog, int version)  // Reflection factory
}
```

### 8.2 Warez Categories

| Category | Warez | Description |
|----------|-------|-------------|
| **ICE Breakers** | Drill, Hammer, Blammo, BudgetPal, LogicBomb | Damage ICE |
| **Virus** (one-shot, self-destruct) | Thunderhead, Python, Acid, Injector | Damage ICE, then delete |
| **Corruptor** (reverses ICE) | Corruptor | Reverses ICE attack pattern |
| **Defensive** | Mimic, Jammies, DoorStop, Concrete, Vaccine | Shield/defense |
| **Special** | Decoder, Sequencer, Probe, Scout, Centurion | Utility |
| **Connection** | ComLink (v1-v7) | Determines which DBs can be accessed |
| **Cyberspace** | Cyberspace (v1,v2,v3,v7) | Allows cyberspace entry |
| **Movement** | EasyRider | Cross zone boundaries in cyberspace |
| **Anti-AI** | BattleChess v4, Hemlock, KuangEleven | Specific AI weaknesses |
| **AI Battle** | Shotgun, KGB (escape) | Use against AIs |
| **Other** | Mindbender, Chaos, PickUpGirls, Toxin, SnailBait, MegaDeath | Various effects |

### 8.3 Key Warez Implementations

**File:** `IceBreakerWarez.java`
```
class IceBreakerWarez extends Warez {
    getRunDuration() -> RUN_FOREVER  // Until manually stopped
    getEffect(gs) -> baseDamage + skillBonus(ICE_BREAKING)
}
```

**File:** `VirusWarez.java`
```
class VirusWarez extends Warez {
    getRunDuration() -> fixedDuration
    abort(gs) -> gameState.eraseSoftware(this)  // Self-destruct on abort
}
```

**File:** `CorruptorWarez.java`
```
class CorruptorWarez extends Warez {
    // Switches ICE animation to ROT mode
    // Reverses ICE damage pattern
}
```

**File:** `ComLinkWarez.java`
```
class ComLinkWarez extends Warez {
    // version determines which databases are accessible
    // version must be >= database.comlink
}
```

**File:** `CyberspaceWarez.java`
```
class CyberspaceWarez extends Warez {
    // Allows entering cyberspace from deck
}
```

---

## 9. DECK SYSTEM

### 9.1 Deck Base Class

**File:** `DeckItem.java`

```
class DeckItem {
    int nSlots;                // Software capacity
    int damage;                // Deck damage (from hits)
    boolean needsRepair;       // If true, deck is damaged
    boolean cyberspaceCapable; // Can enter cyberspace
    
    enum Mode { NONE, CYBERSPACE, LINKCODE }
    Mode mode;
    Warez currentWarez;        // Currently selected/running warez
    ArrayList<Warez> warezStack;  // Push/pop for saving state during battles
    
    int cordX, cordY;          // Cyberspace coordinates
    JackZone zone;             // Current jack zone
    
    void setMode(Mode)
    void pushWarez()           // Save current warez before battle
    void popWarez()            // Restore after battle
    void cleanUp()             // Reset state
}
```

### 9.2 Deck Types

| Deck | File | Slots | Cyberspace | Special |
|------|------|-------|------------|---------|
| UXB | `UXBDeckItem.java` | 8 | No | Shin's pawn shop, can be given or bought |
| ZXB | `ZXBDeckItem.java` | - | No | |
| HikiGaeru | `HikiGaeruDeckItem.java` | - | Yes | |
| Gaijin | `GaijinDeckItem.java` | - | Yes | |
| Bushido | `BushidoDeckItem.java` | - | Yes | |
| Edokko | `EdokkoDeckItem.java` | - | Yes | |
| Katana | `KatanaDeckItem.java` | - | Yes | |
| Tofu | `TofuDeckItem.java` | - | Yes | |
| Shogun | `ShogunDeckItem.java` | - | Yes | |
| 188BJB | `BJB188DeckItem.java` | - | Yes | |
| 350SL | `SL350DeckItem.java` | - | Yes | |
| Cyberspace II | `Cyberspace2DeckItem.java` | - | Yes | |
| Cyberspace III | `Cyberspace3DeckItem.java` | - | Yes | |
| Cyberspace VII | `Cyberspace7DeckItem.java` | - | Yes | |
| Ninja 2000-5000 | `Ninja*DeckItem.java` | varies | Yes | |
| Blue Light Spec. | `BlueLightSpecialDeckItem.java` | - | Yes | |
| Samurai Seven | `SamuraiSevenDeckItem.java` | - | Yes | |

### 9.3 Deck Popup

**File:** `DeckPopup.java`

```
Modes: SOFTWARE, CHOOSE_MODE, ENTER_LINKCODE, ENTER_PASSWORD, RESPONSE, DATABASE

Software list: Show deck warez in groups of 4
Use Software: warez.use(gs) -> if OK, select on deck
  - If cyberspace-capable deck + CyberspaceWarez: enter cyberspace
  - If cyberspace-capable: show connect menu (linkcode or cyberspace)
  - Otherwise: enter link code
Connect Menu: 1. Enter Link Code, 2. Enter Cyberspace
Link Code Entry: Type link code -> dbList.whoIs(code) -> connect or error
  - Checks ComLink version compatibility
  - Opens DatabaseView
```

---

## 10. BODY PARTS

**File:** `BodyPart.java`

```
enum BodyPart {
    HEART(0, "Heart", buy=12000, sell=6000, discount=6600, conDamage=200),
    EYES(1, "Eyes (2)", 10000, 5000, 6500, 150),
    LUNGS(2, "Lungs (2)", 6000, 3000, 3300, 150),
    STOMACH(3, "Stomach", 3000, 1500, 1650, 100),
    ...
    APPENDIX(19, "Appendix", 6, 3, 3, 10);

    final int index;
    final String itemName;
    final int buyPrice;       // Price to buy back
    final int sellPrice;      // Price when selling
    final int discPrice;      // Discounted buy-back price
    final int constDamage;    // Constitution reduction when sold
}
```

**Selling:** Adds to `soldBodyParts`, reduces max constitution, gives `sellPrice` credits.
**Buying back:** Removes from `soldBodyParts`, restores constitution, costs `buyPrice` (or `discPrice` with discount).

**File:** `BodyShopPopup.java`
```
enum Mode { BUY, SELL }
Shows 4 parts per page with prices
use(index):
    BUY: If sold && enough money -> buy back
    SELL: If not sold -> sell
```

---

## 11. CYBERSPACE SYSTEM

### 11.1 Jack Zones

**File:** `JackZone.java`

```
enum JackZone {
    ZERO(0, x=112, y=80),    // Cheap Hotel
    ONE(1, x=416, y=32),     // Gentleman Loser
    TWO(2, x=32, y=176),     // Hitachi/Hosaka/Musabori/Fuji
    THREE(3, x=336, y=128),  // Bank of Berne
    FOUR(4, x=48, y=304),    // Sense/Net
    FIVE(5, x=304, y=304),   // Bank Gemeinschaft
    SIX(6, x=128, y=432),    // No physical jack (EasyRider only)
    SEVEN(7, x=384, y=400);  // Villa Straylight

    static JackZone lookUp(int x, int y)  // From grid coordinates
    static JackZone numToZone(int num)
}
```

### 11.2 Grid System

```
GRID_MAX = 512
GRID_SIZE = 16

Player moves in 16-unit steps within a zone.
Zone change is blocked unless EasyRider is active.
Database positions are fixed in the grid.
```

### 11.3 Cyberspace Popup

**File:** `CyberspacePopup.java`

```
States: EXPLORE, BATTLE

Construction:
    Set deck mode = CYBERSPACE
    Set deck coordinates from room's JackZone
    If room == R50 (Beach): go straight to Neuromancer final fight
    Else: animate initial travel

tick():
    If databaseBattle && databaseBattleBegin:
        setState(BATTLE)
    controlPanel.tick()
    If flatline or Beach/Epilogue door: exit
    If databaseView active: databaseView.tick()
    Else: visualPane.tick()
    If battle ready for database:
        setState(EXPLORE)
        Open DatabaseView
```

### 11.4 Visual Pane

**File:** `VisualPane.java`

```
Components: ExploreGridPane (navigation), BattleGridPane (combat)

Explore Mode:
    Arrow keys move player in grid
    Zone boundary check (blocked without EasyRider)
    Directional animations

Battle Mode:
    Handled by BattleGridPane
```

### 11.5 Battle Grid Pane

**File:** `BattleGridPane.java`

```
IceMode: BASIC, VIRUS, ROT, BROKEN, AI, AI_DEATH, NEUROMANCER, NEUROMANCER_DEATH, NONE

Battle Flow:
    1. ICE battle (BASIC mode)
       - Player attacks with IceBreaker/Virus/Corruptor/Shotgun warez
       - ICE attacks player
       - Virus switches to VIRUS mode (animation change)
       - Corruptor switches to ROT mode
    2. ICE broken (BROKEN -> fade animation)
    3. If DB has AI: AI battle (AI mode)
       - Player attacks with AI battle skills (Philosophy, etc.)
       - Player attacks with anti-AI warez (BattleChess v4, Hemlock, KuangEleven, Shotgun)
       - AI attacks player
       - Weakness hits deal double damage
       - AI says random dialogue on hit
    4. AI defeated (AI_DEATH -> fade animation)
    5. None mode -> Open Database

Neuromancer special:
    First encounter (NEUROMANCER): Transport to Beach
    Final battle: Same as AI battle but with special death monologue
```

### 11.6 Control Panel

**File:** `ControlPanelPane.java`

Shows: Zone, X, Y coordinates, Credits, Damage gauges
Buttons: Inventory (warez), Skills, ROM, Game (disk), Erase, Exit

```
tick():
    updateText()     // Update zone/X/Y/credits display
    updateGauges()   // Update player/opponent damage bars
    If databaseArrived: show Yes/No prompt
```

### 11.7 Damage Gauges

**File:** `DamageGauge.java`

Player damage gauge (vertical): Shows constitution depletion
Opponent damage gauge (horizontal): Shows ICE/AI health remaining

---

## 12. DATABASE SYSTEM

### 12.1 Database Base Class

**File:** `Database.java`

```
abstract class Database {
    final String name;
    final int number;            // DB number
    final int zone;              // Jack zone
    final int comlink;           // Required ComLink version
    final String linkCode;       // Link code for connection
    final String password1;      // Level 1 password
    final String password2;      // Level 2 password
    final String password3;      // Level 3 (sequencer)
    
    final int matrixX, matrixY;  // Position in cyberspace grid
    final Class<? extends AI> aiClazz;  // Associated AI class
    final int ICE_MAX;           // Maximum ICE health
    int ice;                     // Current ICE health
    final int shotDuration = 2500;  // ICE shot animation duration
    final int effect = 100;      // ICE attack damage
    
    LinkedHashMap<Class<Warez>, Integer> warez1, warez2, warez3;  // Downloadable software by access level
    ArrayList<BbsMessage> bbsMessages, bbsMessages2;  // BBS messages
    
    int getIce() / resetIce() / zeroIce()
    AI getAI(GameState gs)       // Lazy instantiation
    int getBattleHealth(GameState gs)  // ICE health if >0, else AI health
    int applyWarezAttack(Warez w, GameState gs)  // Warez damages ICE
    int applySkillAttack(Skill skill, GameState gs)  // Skill damages ICE
    int getEffect(GameState gs)  // ICE attack damage
    void handlePersonListChanged(GameState gs)  // Override for list edits
}
```

### 12.2 Database List

**File:** `DatabaseList.java`

```
class DatabaseList {
    ArrayList<Database> databases;
    
    Database lookup(int dbNumber)
    Database whoIs(String linkCode)    // Find DB by link code
    Database whatsAt(int x, int y)     // Find DB at grid coordinates
}
```

### 12.3 Database Views

**File:** `DatabaseView.java` (abstract)

Each database has a custom view class (e.g., `HosakaDatabaseView`, `KGBDatabaseView`):

```
Modes: LANDING, PASSWORD, PASSWORD_SEQ, PASSWORD_SEQ_DONE,
       CLEAR_WAIT, MAIN,
       MSG_LIST, MSG_SHOW, MSG_SEND,
       VIEW_TEXT,
       PERSON_LIST, PERSON_VIEW,
       SOFTWARE, UPLOAD_DONE

Flow:
    1. LANDING: Show DB heading, press SPACE to continue
    2. PASSWORD: Enter password (1 or 2)
       - If Cyberspace mode: skip to access level 3
       - Correct password -> access level 1 or 2
       - Wrong -> Access denied
    3. PASSWORD_SEQ: Sequencer animation (for level 3 / ComLink 6)
    4. MAIN: Site-specific content (implemented by subclass)
    5. Various sub-modes for messages, text, person lists, software

Common features:
    - Messages (BBS): View, compose, send
    - Downloads: Download warez (filtered by access level)
    - Uploads: Upload warez, check against required type/version
    - Text views: Display static text from DB text resource
    - Person lists: View/edit wanted lists, employee lists
```

### 12.4 Database List

Each database has its own model and view:

| DB | Model File | View File | AI | Zone |
|----|-----------|-----------|----|------|
| Psychologist | `PsychologistDatabase.java` | `PsychologistDatabaseView.java` | Chrome | 0 |
| World Chess | `WorldChessDatabase.java` | `WorldChessDatabaseView.java` | Morphy | 0 |
| Free Matrix | `FreeMatrixDatabase.java` | `FreeMatrixDatabaseView.java` | Sapphire | 1 |
| NASA | `NASADatabase.java` | `NASADatabaseView.java` | HAL | 1 |
| Free Sex Union | `FreeSexUnionDatabase.java` | `FreeSexUnionDatabaseView.java` | Xaviera | 3 |
| Bank of Berne | `BankBerneDatabase.java` | `BankBerneDatabaseView.java` | Gold | 3 |
| Maas Biolabs | `MaasBiolabsDatabase.java` | `MaasBiolabsDatabaseView.java` | Sangfroid | 5 |
| KGB | `KGBDatabase.java` | `KGBDatabaseView.java` | Lucifer | 6 |
| Tessier-Ashpool | `TessierAshpoolDatabase.java` | `TessierAshpoolDatabaseView.java` | Wintermute | 7 |
| Phantom | `PhantomDatabase.java` | `PhantomDatabaseView.java` | Phantom | - |
| Musabori | `MusaboriDatabase.java` | `MusaboriDatabaseView.java` | Greystoke | - |
| Allard | `AllardTechDatabase.java` | `AllardTechDatabaseView.java` | Neuromancer | - |

---

## 13. AI SYSTEM

### 13.1 AI Base Class

**File:** `AI.java`

```
abstract class AI {
    final String name;
    final int index;                // Sprite face index (0-11)
    int constitution;
    final int MAX_CONSTITUTION;
    final Class<? extends Skill> weaknessSkill;   // Weakness skill (double damage)
    final Class<? extends Warez> weaknessWarez;   // Weakness warez
    final int[] TALK;               // Random dialogue indices (4 entries)
    int TALK_SPEC_1;                // Special response 1
    int TALK_SPEC_2;                // Special response 2 (weakness hit)
    
    boolean applySkillAttack(Skill skill, GameState gs):
        if skill matches weaknessSkill: damage = effect * 2, return true
        else: damage = effect
        constitution -= damage
    
    boolean applyWarezAttack(Warez warez, GameState gs):
        damage = warez.getEffect(gs)
        constitution -= damage
        return warez matches weaknessWarez
    
    int getEffect() = MAX_CONSTITUTION / 10  // AI attack damage
}
```

### 13.2 AI Roster

| AI | File | DB | Index | CON | Weakness Skill | Weakness Warez | Talk Indices |
|----|------|----|-------|-----|----------------|----------------|-------------|
| Chrome | `ChromeAI.java` | Psychologist | 0 | 48 | Philosophy | - | 4,5,6,7 |
| Morphy | `MorphyAI.java` | World Chess | - | 96 | Logic | - | 0,1,2,3 |
| Sapphire | `SaphireAI.java` | Free Matrix | - | 192 | Sophistry | - | 8,9,10,11 |
| HAL | `HalAI.java` | NASA | - | 384 | Logic | - | 16,17,18,19 |
| Xaviera | `XavieraAI.java` | Free Sex Union | - | 768 | Phenomenology | - | 20,21,22,23 |
| Gold | `GoldAI.java` | Bank of Berne | - | 1536 | Philosophy | - | 24,25,26,27 |
| Sangfroid | `SangfroidAI.java` | Maas Biolabs | - | 6144 | Phenomenology | - | 28,29,30,31 |
| Lucifer | `LuciferAI.java` | KGB | - | 3072 | Logic | - | 32,33,34,35 |
| Phantom | `PhantomAI.java` | Phantom | - | 24576 | - | BattleChess v4 | 36,37,38,39 |
| Greystoke | `GreystokeAI.java` | Musabori | 10 | 49151 | - | Hemlock | 12,13,14,15 |
| Neuromancer | `NeuromancerAI.java` | Allard | 11 | 49152 | - | KuangEleven | 44-53 |

**CON formula:** `(0x30 << index)`, except:
- Greystoke: `(0x30 << index) - 1`
- Neuromancer: `(0x30 << (index-1))`

### 13.3 AI Battle Mechanics

1. **Normal damage:** Skill/warez deals `getEffect()` damage
2. **Weakness hit:** Skill/warez matches weakness -> damage * 2, AI shows special response (TALK_SPEC_2)
3. **Psychoanalysis:** Occasionally reveals weakness
4. **Zen:** Heal CON (max 2 heals per battle)
5. **KGB warez:** Escape AI battle, teleport to KGB base
6. **On AI defeat:** All AI-battle skills used in the fight are upgraded by 1

---

## 14. PAX SYSTEM

### 14.1 PAX Popup

**File:** `PaxPopupPane.java`

```
Modes: ACCESS, MENU, FIRST, BANKING, NEWS, BBS

1. ACCESS: Enter verification code (any 12-digit code accepted)
2. MENU: 
   1. First Time PAX User Info
   2. Access Banking Interlink
   3. Night City News
   4. Bulletin Board
```

### 14.2 PAX Sub-nodes

| Node | File | Description |
|------|------|-------------|
| First Time | `PaxFirstTimeNode.java` | Scrolling introduction text |
| Banking | `PaxBankingNode.java` | Upload/download credits, transaction history |
| News | `PaxNewsNode.java` | Read news articles |
| BBS | `PaxBbsNode.java` | Read/send BBS messages |

### 14.3 Banking

```
PaxBankingNode Modes: MAIN, UPLOAD, DOWNLOAD, TRANSACTIONS

UPLOAD: Enter amount -> moneyChipBalance -= amount, bankBalance += amount
DOWNLOAD: Enter amount -> moneyChipBalance += amount, bankBalance -= amount
TRANSACTIONS: Show transaction history list
```

---

## 15. SAVE/LOAD SYSTEM

**File:** `GameStateUtils.java`

### 15.1 Format

Java Properties files (`game1.properties` through `game4.properties`):

```
saveModel(gs, slot):
    Properties props = gatherProperties(gs)
    props.store(file, "Javamancer Game Save File. Slot: N")

loadModel(gs, slot):
    Properties loadedProperties.load(file)
    restoreFromProperties(gs, loadedProperties)
```

### 15.2 Saved Fields

```
// Player
name, damage
inventory[].(type, version, etc.)
skills[].(type, level)
bodyParts[].(name)
warez[].(type, version)

// Time
dateMinute, dateHour, dateDay, dateMonth, dateYear

// Money
chipBalance, bankBalance, bankZurichBalance, bankGemeinBalance
bankZurichCreated, bankTransactions[].(date, type, amount)

// Flags
massageInfo1-5, bbsMsgToArmitageSent, bbsMsgFromArmitageRead, ratzPaid
shivaChipMentioned, shivaGaveChip, joystickGiven, gasMaskIsOn
bodyPartDiscount, bankZurichRobbed, comlink2recieved, comlink6uploaded
asanoDiscount, securityPassGiven, larryMoeWanted, psychoProbeCount
hitachiVolunteer, hosakaDaysSincePaid

// Deck/Matrix
deckSlots, aiFightSkill, usingDeck, lastUsedDeck
matrixPosX, matrixPosY, romInstalled

// Room
roomPosX, roomPosY, room (name)
roomsVisited (comma-separated list)
dialogAllowed (comma-separated list)
lockedRooms (format: R12_TR,R22_TL)

// Hotel
hotelCharges, hotelOnAccount, hotelCaviar, hotelSake
hotelDeliverCaviar, hotelDeliverSake

// AI/Persons
aiDefeated (comma-separated class names)
seaWanted[].(name, bama, reason)
chibaWanted[].(name, bama, reason)
hosakaEmployee[].(name, bama, reason)
messageSent[].(to, dbNumber, body, etc.)
```

---

## 16. RESOURCE MANAGEMENT

### 16.1 ResourceManager

**File:** `ResourceManager.java`

```
ResourceManager(File resourceFolder):
    imhFolder = resourceFolder/imh    // Images (sprites)
    picFolder = resourceFolder/pic    // Room backdrops
    anhFolder = resourceFolder/anh    // Animations
    bihFolder = resourceFolder/bih    // Text data (room descriptions, DB text)
    txhFolder = resourceFolder/txh    // Misc text (AITALK, ENDGAME, etc.)

getSprite(String name) -> IMH PNG image (2x scaled)
getBackdrop(Room) -> PIC PNG image (2x scaled)
getRoomText(Room) -> TextResource from BIH file
getDatabaseText(int dbNum) -> TextResource from DB<N> BIH file
getTxhText(String name) -> TextResource from TXH file
getFirstTimeText() -> FTUSER text
getAnimationEntries(Room) -> ANH entry directories
initNewsArticles(list, playerName, dateString)
initPaxBbsMessages(list, playerName)
```

### 16.2 Resource File Formats

**BIH files (Text):** Meta files with text entries separated by comment markers.
**IMH files (Sprites):** Individual PNG images.
**PIC files (Backdrops):** Individual PNG images (room backgrounds).
**ANH files (Animations):** Directory structure with entry folders containing frame sequences.
**TXH files (Misc Text):** Similar to BIH format for global text resources.

### 16.3 Resource Ingest

**File:** `Ingest.java`

Converts original DAT archive files to the folder/PNG structure:
- DAT files contain packed IMH, PIC, ANH, BIH, TXH resources
- Huffman decompression used (see `Huffman.java`)
- `DATMapper.java` maps DAT file offsets

---

## 17. AUDIO SYSTEM

### 17.1 Music Manager

**File:** `MusicManager.java`

```
enum Track {
    TITLE, CHATSUBO, STREET_1, STREET_2, STREET_3,
    MATRIX_1, TESSIER, END_GAME, CREDITS
}

playTrack(Track, volume, startTime, fadeIn_ms, fadeOut_ms)
fadeOutTrack(Track, milliseconds)
stopTrack(Track)
stopAll()
toggleMute()
```

### 17.2 Sound Effects Manager

**File:** `SoundEffectsManager.java`

```
enum Sound {
    PLAYER_FIRE, ICE_HIT, ICE_BROKEN, DENIED, TRANSMIT
}

playTrack(Sound)
```

### 17.3 Room Music

**File:** `RoomMusic.java`

Maps rooms to music tracks with volume, start time, and fade parameters:
```
RoomMusic.get(Room) -> RoomMusic(track, volume, startTime, fadeIn, fadeOut)
```

---

## 18. UI / POPUP SYSTEM

### 18.1 Popup Hierarchy

**File:** `PopupPane.java` (abstract base)

```
abstract class PopupPane {
    PopupListener listener;
    GameState gameState;
    
    abstract boolean handleKeyEvent(KeyEvent)   // Return true to exit
    abstract void cleanup()                      // Called on exit
}

class SmallPopupPane extends PopupPane     // 320x130 at (120,176)
class LargePopupPane extends PopupPane     // 640x288 at (0,0)
```

### 18.2 Popup Types

| Popup | File | Trigger | Description |
|-------|------|---------|-------------|
| Dialog | `DialogPopup.java` | Talk button/NPC | NPC conversation |
| Inventory | `InventoryPopup.java` | Inventory button | Item management |
| Skills | `SkillsPopup.java` | Skills button | View installed skills |
| ROM | `RomPopup.java` | ROM button | ROM construct management |
| Disk | `DiskPopup.java` | Disk button | Save/load disk |
| Deck | `DeckPopup.java` | Deck operation | Software management |
| BodyShop | `BodyShopPopup.java` | BODY_BUY/SELL | Buy/sell organs |
| SkillsVend | `SkillsVendPopup.java` | SKILL_BUY/UPGRADE | Buy/upgrade skills from NPC |
| PawnshopVend | `PawnshopVendPopup.java` | ITEM_BUY | Buy items from NPC |
| SoftwareVend | `SoftwareVendPopup.java` | SOFTWARE_BUY | Buy software from NPC |
| Cyberspace | `CyberspacePopup.java` | DECK->CYBERSPACE | Full cyberspace interface |
| Cryptology | `CryptologyPopup.java` | SKILL_CRYPTO | Cryptology skill interface |

### 18.3 PopupListener Interface

```
interface PopupListener {
    boolean popupExit()           // Close current popup, return if OK to open next
    void popupExit(RoomMode.Popup newPopup)  // Close and open next popup
    void showMessage(String message)         // Show message in description area
}
```

RoomMode implements PopupListener and manages popup stack (only one popup at a time).

### 18.4 RoomMode Button Bar

```
Row 1: [Inventory] [PAX] [Talk]
Row 2: [Skills] [ROM] [Disk]

Status area: [Date] [Time] [Credits] [Constitution]
Cycle through status display modes by clicking.
```

---

## 19. INDEX: JAVA FILE -> FUNCTION MAP

### Model Layer

| File | Primary Responsibility |
|------|----------------------|
| `GameState.java` | All game state data, save/load triggers |
| `GameStateUtils.java` | Properties-based save/load serialization |
| `GameStateDefaults.java` | Default values for save comparison |
| `Item.java` | Item catalog enum, base class |
| `RealItem.java` | Physical items (caviar, passes, etc.) |
| `SkillItem.java` | Skill items (pre-installation) |
| `SoftwareItem.java` | Software items for vending |
| `CreditsItem.java` | Money chip item |
| `DeckItem.java` | Deck base class |
| `BodyPart.java` | 20 body parts with prices/constitution values |
| `Person.java` | Name/BAMA/Reason for person lists |
| `BankTransaction.java` | Bank transaction record |
| `NewsArticle.java` | News article |
| `BbsMessage.java` | BBS message (to/from/body/date) |
| `TextResource.java` | List of text strings from resources |
| `JackZone.java` | 8 jack zones with grid coordinates |
| `Room.java` | 58 rooms enum with extras class refs |
| `RoomMap.java` | Room-to-room connections |
| `RoomBounds.java` | Walk area and door positions |
| `RoomPosition.java` | Player/NPC spawn positions |
| `RoomExtras.java` | Abstract room-specific logic base |
| `DialogCommand.java` | 50+ dialog command codes |
| `Skill.java` | Abstract skill base |
| `*Skill.java` (16 files) | Individual skill implementations |
| `Warez.java` | Abstract warez base |
| `*Warez.java` (50+ files) | Individual warez implementations |
| `*DeckItem.java` (19 files) | Individual deck types |
| `AI.java` | Abstract AI base |
| `*AI.java` (12 files) | Individual AI implementations |
| `Database.java` | Abstract database base |
| `DatabaseList.java` | Database registry |
| `*Database.java` (40+ files) | Individual database implementations |

### View Layer

| File | Primary Responsibility |
|------|----------------------|
| `NeuroGamePane.java` | Main game loop, mode switching |
| `NeuroModePane.java` | Abstract mode base |
| `NeuroModePaneListener.java` | Mode action interface |
| `TitleMode.java` | Title screen |
| `RoomMode.java` | Room gameplay mode |
| `EndGameMode.java` | End game/epilogue mode |
| `ResourceManager.java` | Resource loading |
| `MusicManager.java` | Background music |
| `SoundEffectsManager.java` | Sound effects |
| `PopupListener.java` | Popup callback interface |
| `RoomPane.java` | Room rendering, player movement, animations |
| `PlayerNode.java` | Player character rendering |
| `RoomDescriptionPane.java` | Text description area |
| `RoomMusic.java` | Room-to-music mapping |
| `PopupPane.java` | Abstract popup base |
| `SmallPopupPane.java` | Small popup template |
| `LargePopupPane.java` | Large popup template |
| `DialogPopup.java` | NPC dialog interface |
| `DialogPopupPane.java` | Dialog popup visual base |
| `DialogBubble.java` | Speech bubble rendering |
| `InventoryPopup.java` | Inventory management |
| `SkillsPopup.java` | Skills viewer |
| `RomPopup.java` | ROM management |
| `DiskPopup.java` | Disk save/load |
| `DeckPopup.java` | Deck software management |
| `BodyShopPopup.java` | Body part buy/sell |
| `SkillsVendPopup.java` | Skill vendor |
| `PawnshopVendPopup.java` | Item vendor |
| `SoftwareVendPopup.java` | Software vendor |
| `CryptologyPopup.java` | Cryptology skill |
| `CyberspacePopup.java` | Cyberspace interface |
| `VisualPane.java` | Cyberspace visual (explore + battle) |
| `ExploreGridPane.java` | Cyberspace navigation |
| `BattleGridPane.java` | ICE/AI battle |
| `ControlPanelPane.java` | Cyberspace control panel |
| `SoftwarePane.java` | Cyberspace software selection |
| `DamageGauge.java` | Health bars |
| `YesNoPane.java` | Yes/No prompts |
| `GridPane.java` | Grid rendering base |
| `AiTalkPane.java` | AI speech bubble |
| `DatabaseView.java` | Abstract database UI |
| `*DatabaseView.java` (40+ files) | Individual database UIs |
| `PaxPopupPane.java` | PAX system interface |
| `PaxNode.java` | Abstract PAX sub-node |
| `PaxFirstTimeNode.java` | First-time user info |
| `PaxBankingNode.java` | Banking interface |
| `PaxNewsNode.java` | News reader |
| `PaxBbsNode.java` | BBS interface |
| `EndGameDialog.java` | End game dialog |
| `LoadSaveDialog.java` | Save/load dialog |
| `NameChooserDialog.java` | Name input dialog |

### Room Extras (per-room logic)

| File | Room | NPC/Feature |
|------|------|------------|
| `R1Extras.java` | Chatsubo | Armitage |
| `R2Extras.java` | Street Chatsubo | Street |
| `R3Extras.java` | Cyber Justice | Justice booth |
| `R4Extras.java` | Body Shop | Chin (body parts) |
| `R6Extras.java` | Donut World | |
| `R7Extras.java` | Cheap Hotel | Room service |
| `R8Extras.java` | Gentleman Loser | |
| `R9Extras.java` | Maas Biolabs | |
| `R10Extras.java` | JAL Shuttle | Ticket agent |
| `R11Extras.java` | Zion | |
| `R12Extras.java` | Larry's Software | Larry (CopTalk seller) |
| `R17Extras.java` | Street Maas | Shiva (chip) |
| `R19Extras.java` | Spaceport | Ticket agent (Freeside/Zion) |
| `R20Extras.java` | Marcus Garvey | Tug captain |
| `R22Extras.java` | Villa Straylight | Neuromancer |
| `R23Extras.java` | Panther Moderns | Meeting |
| `R24Extras.java` | Massage Parlor | Akiko (info buyer) |
| `R25Extras.java` | Shin's Pawn | Shin (UXB deck) |
| `R26Extras.java` | Street Light Pole | Zone's girl |
| `R27Extras.java` | Julius Dean | |
| `R28Extras.java` | JAL Shuttle | Return shuttle |
| `R29Extras.java` | Freeside Spacedock | |
| `R32Extras.java` | Metro Holographix | Software vendor |
| `R34Extras.java` | Bank of Berne Lobby | |
| `R35Extras.java` | Bank Manager's Office | |
| `R36Extras.java` | House of Pong | |
| `R40Extras.java` | Crazy Edo's | Edo (ComLink, caviar, sake) |
| `R41Extras.java` | Bank Gemeinschaft | |
| `R44Extras.java` | Asano's | Asano (deck vendor) |
| `R45Extras.java` | Street Security Robot | Robot |
| `R46Extras.java` | Matrix Restaurant | Shiva |
| `R50Extras.java` | Cyberspace Beach | Neuromancer final |
| `R51Extras.java` | Fuji Electric | Wanted list |
| `R52Extras.java` | Security Gate | Gas mask check |
| `R53Extras.java` | Hitachi Biotech | Lung experiment |
| `R54Extras.java` | High Tech Area 1 | |
| `R55Extras.java` | High Tech Area 2 | |
| `R56Extras.java` | SenseNet HQ | ROM checkout |
| `R57Extras.java` | Hosaka HQ | Paycheck |
| `R58Extras.java` | Musabori HQ | Jack only |

---

## APPENDIX: DATA STRUCTURES FOR C PORT

### Recommended C struct equivalents:

```c
// GameState
typedef struct {
    char name[13];
    int damage;
    int money_chip_balance;
    int bank_balance;
    int bank_zurich_balance;
    int bank_gemein_balance;
    int date_minute, date_hour, date_day, date_month, date_year;
    Room current_room;
    int room_pos_x, room_pos_y;
    Door use_door;
    Item inventory[MAX_INVENTORY];
    Skill skills[MAX_SKILLS];
    BodyPart sold_parts[MAX_BODY_PARTS];
    Warez software[MAX_DECK_SLOTS];
    int deck_slots;
    DeckItem *using_deck;
    DeckItem *last_used_deck;
    int matrix_pos_x, matrix_pos_y;
    int rom_installed;
    int ai_fight_skill;
    // ... all flags ...
} GameState;

// Room
typedef struct {
    const char *name;
    RoomExtrasFn extras_fn;
    Door locked_doors;
    Room top, right, bottom, left;
    int walk_t, walk_r, walk_b, walk_l;
    int door_t_x, door_t_w, door_r_y, door_r_w;
    int door_b_x, door_b_w, door_l_y, door_l_w;
    int player_x, player_y, npc_x, npc_y;
    JackZone jack_zone;
    bool has_pax;
} Room;

// Database
typedef struct {
    const char *name;
    int number, zone, comlink;
    const char *link_code;
    const char *password[3];
    int matrix_x, matrix_y;
    int ice_max, ice;
    AI *ai;
    DownloadEntry downloads[3][MAX_DOWNLOADS];  // By access level
    BbsMessage messages[MAX_MESSAGES];
} Database;

// AI
typedef struct {
    const char *name;
    int index;
    int constitution, max_constitution;
    SkillType weakness_skill;
    WarezType weakness_warez;
    int talk[4];
    int talk_spec[2];
} AI;
```

---

*This manual is based on the Javamancer codebase at `/data/nfs/dos/GAMES/8/NEURO/javamancer/`.
All file paths are relative to that directory.*