{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    cmake
    pkg-config
    ninja
    git
  ];

  buildInputs = with pkgs; [
    gcc
    gdb
    
    vulkan-headers
    vulkan-loader
    glfw
    shaderc
    
    # Ultralight dependencies
    gtk3
    cairo
    pango
    glib
    fontconfig
    harfbuzz
    atk
    gdk-pixbuf
    xorg.libX11
    xorg.libXrandr
    xorg.libXcursor
    xorg.libXi
    xorg.libXrender
    xorg.libXext
    bzip2
    fontconfig
    freetype
  ];

  shellHook = ''
    export WOLF_VARS="${pkgs.lib.makeLibraryPath [ 
      pkgs.vulkan-loader 
      pkgs.libGL
      pkgs.glfw 
      pkgs.bzip2 
      pkgs.fontconfig 
      pkgs.gtk3 
      pkgs.atk
      pkgs.pango
      pkgs.gdk-pixbuf
      pkgs.cairo
      pkgs.glib
    ]}:$PWD/../ThirdParty/UltraLight/linux/bin"

    # Fix for the bzip2 version mismatch
    mkdir -p ./.lib_links
    ln -sf ${pkgs.bzip2.out}/lib/libbz2.so.1 ./.lib_links/libbz2.so.1.0
    export WOLF_VARS="$PWD/.lib_links:$WOLF_VARS"
    
    export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
    export LD_LIBRARY_PATH="${pkgs.vulkan-validation-layers}/lib:$LD_LIBRARY_PATH"

    echo "--- Wolf engine environment loaded ---"
    echo "Use LD_LIBRARY_PATH=$WOLF_VARS"
  '';
}
