{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    # Basis-Entwicklungswerkzeuge
    gcc
    cmake
    gnumake
    pkg-config

    # SDL2 1.2 (wie im Projekt benötigt)
    SDL2
    SDL2_image
    
    # wxWidgets (direkt referenziert)
    wxGTK32
    
    # GTK und GSettings-Schemas für GIO
    gsettings-desktop-schemas
    gtk3
    shared-mime-info
    
    # Zusätzliche nützliche Tools
    gdb
    valgrind
    
    # Fehlende Tools
    neovim
  ];

  # Shell-Hook für spezielle Umgebungsvariablen
  shellHook = ''
    # Setze Compiler
    export CC=${pkgs.gcc}/bin/gcc
    export CXX=${pkgs.gcc}/bin/g++
    
    # Setze GSettings-Schemas
    export XDG_DATA_DIRS=${pkgs.gsettings-desktop-schemas}/share:${pkgs.gtk3}/share:${pkgs.shared-mime-info}/share:$XDG_DATA_DIRS
    
    # Setze den Pfad für xgsemu
    if [ -d "build" ] && [ -f "build/xgsemu" ]; then
      export LD_LIBRARY_PATH=$PWD/build:$LD_LIBRARY_PATH
      chmod +x build/xgsemu 2>/dev/null || true
    fi
    
    echo "XGS Emulator Entwicklungsumgebung (C++11)"
    echo "========================================"
    echo "Verfügbare Bibliotheken:"
    echo "- SDL2 $(${pkgs.SDL2.dev}/bin/sdl-config --version)"
    echo "- wxWidgets $(${pkgs.wxGTK32}/bin/wx-config --version 2>/dev/null || echo 'nicht gefunden')"
    echo ""
    echo "Zum Bauen des Projekts:"
    echo "mkdir -p build && cd build && cmake .. && make"
    echo ""
    if [ -f "build/xgsemu" ]; then
      echo "Ausführbare Datei: build/xgsemu"
    fi
  '';

  # Setze LD_LIBRARY_PATH für die Entwicklung, aber nicht PATH
  LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
    pkgs.SDL2
    pkgs.SDL2_image
    pkgs.wxGTK32
    pkgs.gtk3
    pkgs.gsettings-desktop-schemas
  ];
  
  # C++11-Standard verwenden (anstelle von C++98)
  CXXFLAGS = "-std=c++11 -fpermissive -Wno-write-strings";
  
  # GSettings und GIO-Konfiguration
  GIO_EXTRA_MODULES = "${pkgs.glib-networking}/lib/gio/modules";
  GSETTINGS_SCHEMA_DIR = "${pkgs.gsettings-desktop-schemas}/share/glib-2.0/schemas";
  
  # Behalte den bestehenden PATH bei
  NIX_SHELL_PRESERVE_PROMPT = 1;
}
