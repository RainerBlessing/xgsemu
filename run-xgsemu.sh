#!/bin/sh
# run-xgsemu-fixed.sh
# Ein Skript, das versucht, die GSettings- und Pango-Probleme zu beheben

# Stelle sicher, dass das Programm existiert
if [ ! -f "build/xgsemu" ]; then
    echo "Fehler: build/xgsemu nicht gefunden. Bitte kompiliere das Projekt zuerst."
    exit 1
fi

# Deaktiviere Pango und GTK-Warnungen
export PANGO_RC_FILE=/dev/null
export GTK_USE_PORTAL=0
export GTK_THEME=Adwaita:light
export NO_AT_BRIDGE=1

# Deaktiviere GSettings vollständig, wenn möglich
export GSETTINGS_BACKEND=memory
export GIO_USE_VFS=local
export GIO_USE_VOLUME_MONITOR=unix
export GSETTINGS_SCHEMA_DIR=~/.local/share/glib-2.0/schemas

# Setze LD_LIBRARY_PATH
export LD_LIBRARY_PATH="$PWD/build:$LD_LIBRARY_PATH"

# Starte das Programm mit minimalen GTK-Einstellungen
cd build
GTK_CSD=0 ./xgsemu "$@"
