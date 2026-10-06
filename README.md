# ASHEN DEPTHS

An original idle / auto-battler action RPG for the **TI-Nspire CX / CX CAS
(first generation)**, built around the systems of Diablo IV. Your hero
descends endless procedurally generated dungeon floors alone: pathfinding,
fighting, spending resource on core skills and building it back with basic
skills, firing cooldowns, looting and taking the stairs. You shape the
build (skill tree, paragon board, aspects, crafting) and collect offline
gains when you come back.

The systems follow Diablo IV's design. All code, pixel art, names, items
and story are original.

## Status

| Target | State |
|---|---|
| Ndless build `AshenDepths.tns` | Compiles with the official Ndless SDK (GCC 14.2), zero warnings |
| Windows desktop simulator | Builds and runs; every screen checked with the headless renderer |
| Tests | 313,598 checks pass: loot rules, crafting, skill tree, paragon, damage pipeline, saves (v5 plus real v1/v3 saves converted), every translation's format and glyphs, 200 connected floors, 2-hour idle runs for all 6 classes |
| Physical TI-Nspire CX CAS | Earlier versions ran on the owner's calculator; this version is not yet tested on hardware |

## Classes (6) and builds (3 each)

| Class | Resource | Main stat | Builds |
|---|---|---|---|
| Barbarian | Fury | Strength | Whirlwind Bleed, Earthquake, Berserker |
| Sorcerer | Mana | Intelligence | Pyromancer, Cryomancer, Stormcaller |
| Rogue | Energy | Dexterity | Marksman, Venom, Barrage |
| Necromancer | Essence | Intelligence | Bone Spear, Summoner (skeletons), Blood Surge |
| Druid | Spirit | Willpower | Storm, Earth, Werewolf (wolf pack) |
| Spiritborn | Vigor | Dexterity | Eagle Quills, Jaguar, Centipede |

* **Skill tree:** 10 active skills per class in Basic / Core / Defensive /
  Mastery / Ultimate clusters, 6 passives (3 ranks) and 3 key passives (only
  one active). A skill takes 5 ranks, then an Enhancement, then a choice of
  2 Upgrades. Clusters open as you spend points. 6 skills fit on the bar.
* **Resource:** basic skills generate it, core skills spend it, cooldown
  skills fire on their own timers; channels (Whirlwind) drain per pulse.
* **Builds:** each class has three presets. SKILLS > SWITCH BUILD respecs
  into another (free before level 15, then gold, always confirmed). Doing
  anything by hand turns the auto planner off.
* **Minions:** skeleton warriors / mages, spirit wolves. **Corpses** for
  Corpse Explosion. Statuses: freeze, stun, chill, immobilize, burn, poison,
  bleed, shadow. Hero states: barrier, berserking, unstoppable, imbuements.
* Every skill has its own animated effect, tinted by its current element.

## Damage (Diablo IV buckets)

`weapon x skill% x (1 + main stat/1000) x (1 + sum of +% bonuses) x each [x]
multiplier x Vulnerable (1.2 + vulnerable damage) x Critical (1.5 + crit
damage) x Overpower ((+ life + barrier) x (1.5 + overpower damage))`.
Lucky hits trigger affix effects. Damage numbers are colour coded: white
normal, yellow critical, purple vulnerable, pink both, cyan (big) overpower,
orange overpower + critical, DoTs in their element colour. HERO > DEL shows
the full character sheet and the biggest hit of the floor broken down by
bucket.

## Loot and gold

* Common, Magic (1 affix), Rare (2), Legendary (3 + an aspect), Unique (4
  fixed + a unique power), Mythic Unique. 40 affix types with slot pools;
  every weapon type, ring, amulet and boot has an implicit affix.
* **Ancestral** items (deep floors) have 1-3 greater affixes (x1.5, shown
  with a star) and +100 item power. Legendary and better drops stand in a
  **pillar of light** (orange / gold / purple, taller for ancestral) with a
  banner announcing them.
* **Codex of Power:** every legendary aspect found is remembered (best roll).
* **Town** (where the gold goes): Blacksmith (masterwork 12 ranks, temper 2
  extra affixes from 6 recipes), Occultist (enchant one affix: pick 1 of 2
  or keep; imprint codex aspects), Jeweler (7 gem kinds x 5 tiers, sockets,
  combining), Alchemist (potion upgrades, 30-minute elixirs), Gambler (buy
  unknown gear by slot). Salvaging gives iron and souls.
* Auto craft (option) masterworks, tempers, imprints build aspects,
  sockets gems and keeps an elixir running.

## Paragon

From level 50 (cap 60) paragon points: four 15x15 boards per class with
normal, magic, rare and legendary nodes, a glyph socket on each board and a
gate to the next. Glyphs scale with the main stat in their radius and level
up from Torment guardians. Auto paragon (option) plans the boards.

## Your hero

New game: choose a class, then create the hero (skin, hair style and
colour, face, eyes, clothing colour, name). Equipped helm, chest (with
pauldrons from chain mail up), gloves, pants, boots, weapon and off-hand are
all drawn on the hero in their material colour. OPTIONS > APPEARANCE changes
the look later; SHOW HELM hides the helm.

## Story

An original five-act campaign (Cindermere under the ash). The story plays
as subtitles over the battle; nothing waits for a key press. OPTIONS >
STORY switches to full pages, OPTIONS > JOURNAL rereads chapters.

## Languages

English, 简体中文, 繁體中文, 日本語 and 한국어. Change it on the title screen
(LANGUAGE row, ENTER or LEFT/RIGHT) or in OPTIONS > LANGUAGE; each save
keeps its own language.

Translations live in `tools/i18n/*.tsv` (English key, then zh-Hans, zh-Hant,
ja, ko; `//` starts a comment). `python tools/i18n/gen_i18n.py` turns them
into `src/i18n/i18n_data.c` and embeds only the glyphs the text uses; it
fails if a translation changes a printf conversion or uses a character the
font lacks (it reads the 8px monospaced BDF files of the font release from
`.tools/fonts/`). `python tools/i18n/extract.py --missing` lists on-screen English
strings that have no translation yet.

CJK text uses the [Fusion Pixel Font](https://github.com/TakWolf/fusion-pixel-font)
8px (c) TakWolf and contributors, SIL Open Font License 1.1; the licence and
those of the fonts it builds on are in `assets/fonts/`.

## Saving

* 3 save slots, CONTINUE / LOAD GAME / NEW GAME, delete with confirmation.
* Saves from earlier versions load automatically: class, level, gold,
  floors, embers, story and options carry over; old gear is re-forged into
  new items of the same slot, rarity and item level; skill points are
  refunded and the closest build is rebuilt. Converted saves cannot be
  opened by the old version any more (current format: v5).
* Autosave every 2 minutes and on exit; CRC-checked, written atomically.

## Controls

| Action | Calculator | Desktop |
|---|---|---|
| Open menu / next page | TAB or MENU | Tab or M |
| Options page | ESC (in battle) | Esc |
| Navigate | arrows or 8/2/4/6 | arrows or WASD |
| Select / learn / buy / equip | ENTER, click or 5 | Enter or Space |
| Secondary (salvage, upgrade, temper, next board, sheet) | DEL | Delete or X |
| Third (lock item, skill on bar, glyph, combine gems) | CTRL | Ctrl or L |
| Back | ESC | Esc |
| FPS overlay | F | F |

## Install on the calculator

Ndless must already be installed. Copy `AshenDepths.tns` into the `ndless`
folder with TI-Nspire Computer Link (replace the old one), then open it from
My Documents. Saves `AshenDepths1.sav.tns` to `AshenDepths3.sav.tns` sit
next to it and are converted on first load.

## Build

```bash
sh tools/build_nspire.sh    # -> dist/AshenDepths.tns (Docker image bitling/ndless-sdk)
sh tools/build_host.sh      # -> build/AshenDepthsDesktop.exe, tests.exe, ad_headless.exe, ad_sim.exe
build/tests.exe
build/ad_sim.exe 2          # 2 idle hours for each of the 18 builds
```

Curves and prices live in `src/game/balance.c`; skill numbers in
`src/game/skills.c`; aspects and uniques in `src/game/aspects.c`.
