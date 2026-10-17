# Umbau von Cpu::execute

`Cpu::execute` hat CC 213 (CRAP 2007): ein einziger `switch` mit rund 40-fach
kopiertem Adressierungs- und Flag-Code. Alle Fehler der letzten Zeit (Carry bei
`jmp PC+W`, `retp`-Flags, Zyklenzahlen) steckten dort.

Reiner Umbau: Das Verhalten bleibt bitgenau gleich, auch bei bekannten
Eigenheiten. Fehler werden danach in eigenen Commits mit Tests behoben.

## Stage 0: Undefiniertes Verhalten beseitigen
**Goal**: `clr fr` benutzt die nie gesetzte Variable `bit`, damit ist das Verhalten
nicht deterministisch. Ohne das ist kein Vergleichstest möglich.
**Success Criteria**: `clr RE` schreibt Schwarz ans Video, `clr RA` ruft den
Joystick-Code auf und nicht das Video, `clr` auf ein normales Register hat keine
Nebenwirkung.
**Tests**: `CpuTest.ClrReWritesBlackToVideo`, `ClrRaDoesNotWriteToVideo`,
`ClrNormalRegisterDoesNotWriteToVideo`
**Status**: Complete

## Stage 1: Regressionsnetz
**Goal**: Jede Abweichung vom heutigen Verhalten fällt sofort auf.
**Success Criteria**: Ein eingefrorener Stand der heutigen CPU
(`tests/reference/`) läuft im Test neben der echten CPU. Verglichen wird der
komplette Zustand nach jedem Befehl.
**Tests**: Alle 4096 Opcodes mit Zufallszuständen, außerdem die 17 Beispielprogramme
über mehrere Frames (Zustand, Videofragmente, Bild, SRAM, Soundchip).
Gegenprobe: 6 absichtlich eingebaute Fehler in `cpu.cpp` wurden alle gefunden.
**Status**: Complete

## Stage 2: Umbau
**Goal**: `execute` in kleine Funktionen zerlegen (Decode, Adressierung, ALU,
Bit-Befehle, Sprünge, Spezialbefehle), die Kopien in Hilfsfunktionen ziehen.
**Success Criteria**: Jeder Schritt ist ein Commit, der kompiliert und alle Tests
inklusive Vergleichstest besteht. CC der größten Funktion unter 20.
**Tests**: Vergleichstest aus Stage 1 nach jedem Schritt.
**Status**: Not Started

## Stage 3: Aufräumen
**Goal**: Gerüst entfernen, bekannte Fehler dokumentieren.
**Success Criteria**: `tests/reference/` und der Vergleichstest sind entfernt
(sie vergleichen sonst gleichen Code), Tests je Befehlsfamilie für Ergebnis und
Flags bleiben. Gefundene Fehler stehen in einer Liste mit Test.
**Status**: Not Started
